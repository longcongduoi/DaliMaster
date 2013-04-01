#include <avr/io.h>
#include <util/delay.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "dali.h"
#include "suart.h"
#include "Interpreter.h"

const char* username = "";
const char* password = "";

int main()
{
	//input buffer for terminal
	byte buffer[MAX_BUFFER_LENGTH] = {[0 ... MAX_BUFFER_LENGTH-1] = 0};
	fifo_t fifo;	
	word frame;
	int ret;
	int i;

	//buffer for temporary strings
	byte temp[MAX_BUFFER_LENGTH+1];
	memset(&temp, 0, MAX_BUFFER_LENGTH+1); //clear temp buffer for new strings

	suart_init();
	dali_init();
	fifo_init(&fifo, buffer, MAX_BUFFER_LENGTH);

	//wait until 7 sec no messages via uart --> boot time has ended
	for(i = 0; i < 700; i++)
	{
		if(suart_getc_nowait () > -1)
			i = 0;
		
		_delay_ms(10);
	}

	
	//now login
	if(username[0] != 0)
	{
		suart_putstring(username);
		suart_putc('\n');
		_delay_ms(1000);
		
		suart_putstring(password);		
	}
	
	//if no username exist ---> simply login via pressing enter (no password is set in openwrt router)
	suart_putc('\n');

	wdt_enable(WDTO_2S );

	for(;;)
	{
		wdt_reset();		
		byte c = suart_getc_wait () & 0xFF;
	  	fifo_put(&fifo, c);
		if(c == '\n') //'\n' is end of command
		{
			_inline_fifo_get_chars( &fifo, temp, fifo.count);
			if(ret = decode_command_to_frame(temp, &frame) >= 0)
			{
				if(_MODE_SIMPLE_ == ret)
				{ 
					if(_ERR_OK_ == dali_send(frame))
						suart_putstring("ACK\n");
					else
						suart_putstring("ERROR\n");
				}
				else if(_MODE_REPEAT_TWICE_ == ret)
				{
					if(_ERR_OK_ == dali_send_with_repeat(frame))
						suart_putstring("ACK\n");
					else
						suart_putstring("ERROR\n");
				}
				else if(_MODE_QUERY_ == ret)
				{
					byte ans;
					if(_ERR_OK_ == dali_query(frame, &ans))
					{
						suart_putstring("ANS ");
						suart_putc(nibble_to_ascii(ans >> 4));
						suart_putc(nibble_to_ascii(ans & 0x0F));
						suart_putc('\n');
					}
					else
						suart_putstring("ERROR\n");
				}
			}
			else
			{
				suart_putstring("NACK\n");
			}

			memset(&temp, 0, MAX_BUFFER_LENGTH+1); //clear temp buffer for new strings
		}		
	}
	
	return 0;
}
