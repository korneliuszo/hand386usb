/*
 * hello.cpp
 *
 *  Created on: Jun 24, 2023
 *      Author: kosa
 */

#include <vmm.hpp>
#include <gcc.h>
#include <stdbool.h>
#include <dev_vxd_dev_vmm.h>
#include <vxdcall.hpp>
#include <stdio.h>

#include <usbdesc.hpp>
#include <ch375.hpp>
#include <mouse.hpp>

Mouse mouse;
CH375 ch375(0x260);

extern "C"
[[gnu::section(".ddb"), gnu::used, gnu::visibility ("default")]]
 const DDB DDB = Init_DDB((Device_ID)USBCH375_Device_ID,
		 1, 0, "CH375MSE", Init_Order::Undefined_Init_Order);

static inline void outb(const uint16_t port, uint8_t val)
{
	asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

void delayticks(uint64_t ticks) //in 0.8us
{
	uint64_t end = VTD_Get_Real_Time() + ticks;
	while(VTD_Get_Real_Time() < end);
}

volatile vxd_timeout_handle_t irq_timeouter;

void usb_irq_handler();


uint8_t endp6_mode;

void toggle_recv()
{
	ch375.set_endp6(endp6_mode);
	endp6_mode^=0x40;
	//delayticks(4/0.8);
}


void mouse_complete(void * obj)
{
}

volatile static bool connected = false;
enum class State {
	DESCR1,
	ADDR,
	DESCR2,
	CONFIGURING,
	CONFIGURED,
	IN_PKT_SCHEDULE,
	SENT_IN_PKT,
	FAIL,
};
volatile static State state;

void connect_timeout()
{
	uint8_t dev_data_rate = ch375.get_dev_rate();

	ch375.set_usb_mode(7);

	delayticks(55000/0.8); //TODO: async

	ch375.set_usb_mode(6);

	if(dev_data_rate&0x10) //LS
	{
		ch375.set_usb_speed(0x02);
		Out_Debug_String("LS\r\n");
	}
	else // HS
	{
		ch375.set_usb_speed(0x00);
		Out_Debug_String("HS\r\n");
	}

	ch375.set_retry(0x05);

	delayticks(72000/0.8); //TODO: async

	ch375.get_desr(1);
	state = State::DESCR1;
	connected = true;
	irq_timeouter = Set_Global_Time_Out(1, 0, (const void*)saved_flags<usb_irq_handler>);
}

void initialize()
{
	irq_timeouter = 0;

#if 0
	{
		char buf[16];
		snprintf(buf,16,"Ver: 0x%02x\r\n",ch375.get_version());
		Out_Debug_String(buf);
	}
#endif
	ch375.set_usb_mode(5);
	connected = false;
	state = State::FAIL;
	irq_timeouter = Set_Global_Time_Out(1, 0, (const void*)saved_flags<usb_irq_handler>);
}

void usb_irq_handler()
{
	static uint8_t in_ep;
	static uint8_t interface;
	static uint8_t config;
	uint8_t status = ch375.get_status();
	irq_timeouter = 0;

#if 0
	{
		char str[64];
		snprintf(str,364,"Sts: 0x%02x %d\r\n",
				status, (int)state);
		Out_Debug_String(str);
	}
#endif

	if(status == 0x15)
	{
		if(!connected)
		{
#ifndef NDEBUG
			Out_Debug_String("Connected\r\n");
#endif
			Set_Global_Time_Out(100, 0, (const void*)saved_flags<connect_timeout>);
			return;
		}
	}
	else if(status == 0x16)
	{
		if(connected)
		{
#ifndef NDEBUG
			Out_Debug_String("Disconnected\r\n");
#endif
			connected=false;
			ch375.set_usb_mode(5);
			state=State::FAIL;
		}
	}
	else if (status == 0x14)
	{
		switch(state)
		{
		case State::DESCR1:
			ch375.rd_usb_data(nullptr, 0);
			ch375.set_address(4);
			state = State::ADDR;
			break;
		case State::ADDR:
			ch375.rd_usb_data(nullptr, 0); //TODO: NOT IN specs
			ch375.set_usb_addr(4);
			ch375.get_desr(2);
			state = State::DESCR2;
			break;
		case State::DESCR2:
		{
			uint8_t descr[64];
			int len = ch375.rd_usb_data(descr, 64);
			if(0)
			{
				char buf[16];
				snprintf(buf,16,"Desc %d\r\n",len);
				Out_Debug_String(buf);
			}
			uint8_t * descr_ptr = descr;
			bool found_config = false;
			bool found_iface = false;
			while (len>0)
			{
				USBDESC::usb_desc_header * desc = (USBDESC::usb_desc_header *)descr_ptr;
				if(desc->bDescriptorType == (uint8_t)USBDESC::USB_DESC_TYPE::CONFIGURATION)
				{
					USBDESC::usb_config *cfg_desc = (USBDESC::usb_config *)desc;
					config = cfg_desc->bConfigurationvalue;
				}
				else if(desc->bDescriptorType == (uint8_t)USBDESC::USB_DESC_TYPE::INTERFACE)
				{
					USBDESC::usb_interface *i_desc = (USBDESC::usb_interface *)desc;
					if(i_desc->bInterfaceClass == 3 &&
							i_desc->bInterfaceSubClass == 1 &&
							i_desc->bInterfaceProtocol == 2)
					{
						interface = i_desc->bInterfaceNumber;
						found_iface = true;
					}
					else
					{
						found_iface = false;
					}
				}
				if(desc->bDescriptorType == (uint8_t)USBDESC::USB_DESC_TYPE::ENDPOINT)
				{
					USBDESC::usb_endpoint *cfg_e = (USBDESC::usb_endpoint *)desc;
					if(found_iface && (cfg_e->bEndpointAddress&0x80))
					{
						in_ep = cfg_e->bEndpointAddress;
						found_config = true;
						break;
					}

				}
				descr_ptr+= desc->bLength;
				len -=desc->bLength;
			}
			if (!found_config)
			{
				Out_Debug_String("Cannot into descr\r\n");
				state=State::FAIL;
				connected=false;
				break;
			}
			//in_ep = 0x81;
			//interface = 0;
			//config = 1;
			state = State::CONFIGURING;
		}
		/* no break */
		case State::CONFIGURING:
		{
			uint8_t set_conf[8] =
			{0x00,0x09,config,0x00,0x00,0x00,0x00,0x00};
			ch375.wr_usb_data_7(set_conf,8);
			ch375.issue_token_x(0x00,ch375.Token::DEF_USB_PID_SETUP,0x00);
			do
			{
				status = ch375.get_status();
			}
			while(status!=0x14 && ((status&0x20) != 0x20));
			if (status != 0x14)
				break;
			ch375.issue_token_x(0x80,ch375.Token::DEF_USB_PID_IN,0x00);
			state = State::CONFIGURED;
		}
		break;
		case State::CONFIGURED:
		{
			endp6_mode = 0x80; // reset the sync flags
			uint8_t set_proto[8] =
			{0x21,0x0b,0x00,0x00,interface,0x00,0x00,0x00};
			ch375.wr_usb_data_7(set_proto,8);
			ch375.issue_token_x(0x00,ch375.Token::DEF_USB_PID_SETUP,0x00);
			//delayticks(100/0.8);
			do
			{
				status = ch375.get_status();
			}
			while(status!=0x14 && ((status&0x20) != 0x20));
			if (status != 0x14)
				break;
			ch375.issue_token_x(0x80,ch375.Token::DEF_USB_PID_IN,0x00);
			state = State::IN_PKT_SCHEDULE;
			break;
		}
		break;
		case State::IN_PKT_SCHEDULE:
			toggle_recv();
			ch375.issue_token(ch375.Token::DEF_USB_PID_IN,in_ep);
			state = State::SENT_IN_PKT;
			break;

		case State::SENT_IN_PKT:

		{
			uint8_t report[3];
			uint8_t len = ch375.rd_usb_data(report, 3);
			toggle_recv();
			ch375.issue_token(ch375.Token::DEF_USB_PID_IN,in_ep);

			static int32_t mx =0x7FFF;
			static int32_t my =0x7FFF;
			static uint8_t old_buttons = 0;

			if(len == 3 && (report[1] || report[2] || (report[0]!=old_buttons)))
			{
				if(0)
				{
					char buff[30];
					snprintf(buff,30,"REP 0x%02x %d %d\r\n",
							report[0],
							report[1],
							report[2]);
					Out_Debug_String(buff);
				}
				mx+=static_cast<int16_t>(static_cast<int8_t>(report[1]))*3<<5;
				if(mx > 0xffff)
					mx = 0xffff;
				if(mx < 0)
					mx = 0;
				my+=static_cast<int16_t>(static_cast<int8_t>(report[2]))*4<<5;
				if(my > 0xffff)
					my = 0xffff;
				if(my < 0)
					my = 0;
				old_buttons = report[0];
				mouse.Set_Mouse_Position(
						{nullptr,nullptr},
						mx, my, old_buttons);
			}
		}
		break;
		case State::FAIL:
			break;
		}
	}
	else if(status < 0x10 || status == 0x51)
	{
		//continue
	}
	else if(state == State::SENT_IN_PKT)
	{
		if(status == 0x2a)
		{
			ch375.issue_token(ch375.Token::DEF_USB_PID_IN,in_ep);
			state = State::SENT_IN_PKT;
		}
		else
		{
			Out_Debug_String("RESTALL\r\n");
			ch375.clear_stall(in_ep);
			endp6_mode = 0x80;
			state = State::IN_PKT_SCHEDULE;
		}
	}
	else
	{
		if(connected)
		{
			Out_Debug_String("Something went wrong\r\n");
			{
				char str[64];
				snprintf(str,364,"Sts: 0x%02x %d\r\n",
						status, (int)state);
				Out_Debug_String(str);
			}
		}

		status = ch375.test_connect();
		if(status == 0x16)
		{
			if(connected)
			{
#ifndef NDEBUG
				Out_Debug_String("Disconnected\r\n");
#endif
				state=State::FAIL;
				connected=false;
			}
		}
	}

	irq_timeouter = Set_Global_Time_Out(connected?1:500, 0, (const void*)saved_flags<usb_irq_handler>);
}

bool Device_Init(uint32_t cmdtail, uint32_t sysVM, uint32_t crs)
{
	bool exists = mouse.Init(sysVM, crs);
	if(!exists)
		return 0;

	if(!ch375.check_exist())
	{
		Out_Debug_String("CH375 not found\r\n");
		return 0;
	}

	{
		char buf[16];
		snprintf(buf,16,"Ver: 0x%02x\r\n",ch375.get_version());
		Out_Debug_String(buf);
	}

	ch375.reset();

	state=State::FAIL;

	irq_timeouter = Set_Global_Time_Out(450, 0, (const void*)saved_flags<initialize>);

	return 1;
}

bool Focus(uint32_t VID, uint32_t flags, uint32_t VM, uint32_t crs)
{
	mouse.Focus(VID, flags, VM);
	return 1;
}

void Crit_Init(){
	Out_Debug_String("Hello from ch375mse by Kaede\r\n");
}

[[gnu::section(".vxd_control"), gnu::used]]
 static const Control_callback ahndlr =
		 Init_Control_callback
		 <System_Control::Device_Init,Device_Init,'S','b','B'>();

[[gnu::section(".vxd_control"), gnu::used]]
 static const Control_callback bhndlr =
		 Init_Control_callback
		 <System_Control::Set_Device_Focus,Focus,'d','S','b','B'>();
