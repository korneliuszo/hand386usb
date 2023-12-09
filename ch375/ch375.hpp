/*
 * ch375.hpp
 *
 *  Created on: Dec 9, 2023
 *      Author: kosa
 */

#ifndef CH375_CH375_HPP_
#define CH375_CH375_HPP_

#include <stdint.h>

const uint16_t USBCH375_Device_ID = 0xF102;

class CH375{

	static inline void outb(uint16_t port, uint8_t val)
	{
		asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
	}

	static inline uint8_t inb(uint16_t port)
	{
		uint8_t ret;
		asm volatile ( "inb %1, %0"
				: "=a"(ret)
				  : "Nd"(port)
					: "memory");
		return ret;
	}

	static inline void io_wait(unsigned char point)
	{
		asm volatile ( "outb %0, $0x80" : : "a"(point) );
	}

	const uint16_t CH375_CMD_ADDR;
	const uint16_t CH375_DATA_ADDR;

	void CH375_WR_CMD_PORT (uint8_t cmd)
	{
		//delayticks(3/0.8);
		outb(CH375_CMD_ADDR,cmd);
		io_wait(0x00);
		//delayticks(3/0.8);
	}

	void CH375_WR_DAT_PORT(uint8_t dat)
	{
		outb(CH375_DATA_ADDR,dat);
		io_wait(0x00);
		//delayticks(2/0.8);
	}

	uint8_t CH375_RD_DAT_PORT()
	{
		//delayticks(2/0.8);
		return inb(CH375_DATA_ADDR);
	}


public:
	CH375(uint16_t base)
	: CH375_CMD_ADDR(base+1)
	, CH375_DATA_ADDR(base)
	{

	}

	uint8_t rd_usb_data(uint8_t *buf, uint8_t len)
	{
		CH375_WR_CMD_PORT(0x28);//RD_USB_DATA
		uint8_t rlen=CH375_RD_DAT_PORT();
		len = std::min(rlen,len);
		uint8_t retlen = len;
		rlen -=len;
		while(len--)
			*buf++=CH375_RD_DAT_PORT();
		while(rlen--)
			CH375_RD_DAT_PORT();
		return retlen;
	}


	void wr_usb_data_7(const uint8_t *buf, uint8_t len)
	{
		CH375_WR_CMD_PORT(0x2B);//WR_USB_DATA7
		CH375_WR_DAT_PORT(len);
		while(len--)
			CH375_WR_DAT_PORT(*buf++);
	}

	enum class  Token : uint8_t
	{
				DEF_USB_PID_SETUP = 0x0D,
				DEF_USB_PID_OUT   = 0x01,
				DEF_USB_PID_IN    = 0x09,
				DEF_USB_PID_SOF   = 0x05,
	};

	void issue_token(Token token, uint8_t endpoint)
	{
		CH375_WR_CMD_PORT(0x4F);//ISSUE_TOKEN
		CH375_WR_DAT_PORT(((uint8_t)token) | ((endpoint&0xf)<<4));
	}

	void issue_token_x(uint8_t sync, Token token, uint8_t endpoint)
	{
		CH375_WR_CMD_PORT(0x4E);//ISSUE_TOKEN_X
		CH375_WR_DAT_PORT(sync);
		CH375_WR_DAT_PORT(((uint8_t)token) | ((endpoint&0xf)<<4));
	}

	void set_endp6(uint8_t endp6_mode)
	{
		CH375_WR_CMD_PORT(0x1c);//SET_ENDP6
		CH375_WR_DAT_PORT(endp6_mode);
	}

	void get_desr(uint8_t type) {
		CH375_WR_CMD_PORT(0x46); //GET_DESCR
		CH375_WR_DAT_PORT(type); /* description type, only 1(device) or 2(config) */
	}

	void set_config(uint8_t cfg) {
		CH375_WR_CMD_PORT(0x49); //SET_CONFIG
		CH375_WR_DAT_PORT(cfg);
	}

	uint8_t get_dev_rate()
	{
		CH375_WR_CMD_PORT(0x0a);
		CH375_WR_DAT_PORT(0x07);
		return CH375_RD_DAT_PORT();
	}

	void set_usb_mode(uint8_t mode)
	{
		CH375_WR_CMD_PORT(0x15);//SET_USB_MODE
		CH375_WR_DAT_PORT(mode);
	}

	bool get_usb_mode_completion()
	{
		return CH375_RD_DAT_PORT();
	}

	void set_usb_speed(uint8_t speed)
	{
		CH375_WR_CMD_PORT(0x04); //SET_USB_SPEED
		CH375_WR_DAT_PORT(speed);
	}

	void set_retry(uint8_t retries)
	{
		CH375_WR_CMD_PORT( 0x0B ); //SET_RETRY
		CH375_WR_DAT_PORT( 0x25 );
		CH375_WR_DAT_PORT( retries );
	}

	uint8_t get_version()
	{
		CH375_WR_DAT_PORT(0x01); // after reset ver doesn't work
		return CH375_RD_DAT_PORT();
	}

	uint8_t get_status()
	{
		CH375_WR_CMD_PORT(0x22);//GET_STATUS
		return CH375_RD_DAT_PORT();
	}

	void set_address(uint8_t addr)
	{
		CH375_WR_CMD_PORT(0x45);//SET_ADDRESS
		CH375_WR_DAT_PORT(addr);
	}

	void set_usb_addr(uint8_t addr)
	{
		CH375_WR_CMD_PORT(0x13);//SET_USB_ADDR
		CH375_WR_DAT_PORT(addr);
	}

	void clear_stall(uint8_t ep)
	{
		CH375_WR_CMD_PORT(0x41); //CLR_STALL
		CH375_WR_DAT_PORT(ep);
	}

	uint8_t test_connect()
	{
		CH375_WR_CMD_PORT(0x16); //TEST_CONNECT
		uint8_t status;
		do {
			status = CH375_RD_DAT_PORT();
		} while (status == 0);
		return status;
	}

	bool check_exist()
	{
		CH375_WR_CMD_PORT(0x06);
		CH375_WR_DAT_PORT(0xA5);
		return (CH375_RD_DAT_PORT() == 0x5A);
	}

	void reset()
	{
		CH375_WR_CMD_PORT(0x05);//RESET
	}

};






#endif /* CH375_CH375_HPP_ */
