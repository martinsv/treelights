#include "treelights.h"

#define GPIO_ADDR 0x18040000 
#define GPIO_BLOCK 48
#define GPIO_P1 18
#define GPIO_N1 19
#define GPIO_P2 20
#define GPIO_N2 23
#define GPIO_OE 24
#define GPIO_ON 0
#define GPIO_OFF 1
#define GPIO_OUT 1
#define GPIO_IN 0

#define PWM_PERIOD 30 // microseconds

#define SOCKET_NAME "/tmp/treelights.sock"

#define PROGRAMS 6

int gpio[5] = {GPIO_P1, GPIO_N1, GPIO_P2, GPIO_N2, GPIO_OE};

int prg[PROGRAMS][4][4] = {
	{ {0, 1, 0, 0}, {99, -1, 0, 0}, {0, 0, 0, 1}, {0, 0, 99, -1} },
	{ {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 1, 0, 0}, {0, 0, 0, 1} },
	{ {99, -1, 0, 1}, {0, 1, 99, -1}, {99, -1, 0, 1}, {0, 1, 99, -1} },
	{ {99, 0, 0, 0}, {99, 0, 0, 0}, {99, 0, 0, 0}, {99, 0, 0, 0} },
	{ {0, 0, 99, 0}, {0, 0, 99, 0}, {0, 0, 99, 0}, {0, 0, 99, 0} },
	{ {49, 0, 50, 0}, {49, 0, 50, 0}, {49, 0, 50, 0}, {49, 0, 50, 0} }
};

int main(int argc, char * argv[])
{
    int program = 0;
    float brightness = 1;
    int speed = 2;

    // signal handling
    do_exit = 0;
    static struct sigaction act; 
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = SIG_IGN;
    sigaction (SIGHUP, &act, NULL);
    act.sa_handler = cleanup;
    sigaction (SIGINT, &act, 0);
    act.sa_handler =  cleanup;
    sigaction (SIGTERM, &act, 0);
    act.sa_handler =  cleanup;
    sigaction (SIGKILL, &act, 0);
    // ---
	
    // UNIX socket
    mode_t mask = umask(S_IXUSR | S_IXGRP | S_IXOTH);
    int s, s2, len;
    unsigned int t;
    struct sockaddr_un local, remote;
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket error\n");
        return -1;
    }
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK); 
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCKET_NAME);
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if (bind(s, (struct sockaddr *)&local, len) == -1)
    {
        printf("Socket bind failed\n");
        return -1;
    }
    if (listen(s, 5) == -1) {
        printf("Socket listen failed\n");
        return -1;
    }
    umask(mask);
	// ---
	
	int fd;
	
	// get direct access to GPIO registers through /dev/mem
	if (gpioSetup())
	{
		printf("Error accessing GPIO registers\n");
		return -1;
	}
	
	// set GPIO direction
	for (int i=0; i<5; i++)
	{
		gpioDirection(gpio[i], GPIO_OUT);
	}
	
	gpioSet(GPIO_OE, GPIO_ON); // enable LED driver outputs
	
	char sockinput[25];
	char * cmd[2];
	int tick = 0;
	int tock = 0;
	int count = 0;
	int gpioValuePos = prg[program][tock][0];
	int directionPos = prg[program][tock][1];
	int gpioValueNeg = prg[program][tock][2];
	int directionNeg = prg[program][tock][3];
	
	while(!do_exit)
	{
		// PWM
		for (int j = 0; j < 2; j++)
		{
			if (gpioValuePos > 0)
			{
				gpioSet(gpio[2*j], GPIO_ON);
				gpioSet(gpio[2*j+1], GPIO_OFF);
			}
			else
			{
				if (gpioValueNeg == 99)
				{
					gpioSet(gpio[2*j], GPIO_OFF);
					gpioSet(gpio[2*j+1], GPIO_ON);
				}
				else
				{
					gpioSet(gpio[2*j], 1);
					gpioSet(gpio[2*j+1], GPIO_OFF);
				}
			}
		}

		for (int i = 0; i < 100; i++)
		{   
			for (int j = 0; j < 2; j++)
			{
				if (i == floor((float)gpioValuePos/brightness))
				{
					gpioSet(gpio[2*j], GPIO_OFF);
					gpioSet(gpio[2*j+1], GPIO_OFF);
				}
				if (i == (100 - floor(((float)gpioValueNeg/brightness))))
				{
					gpioSet(gpio[2*j], GPIO_OFF);
					gpioSet(gpio[2*j+1], GPIO_ON);
				}
			}
			_usleep(PWM_PERIOD);
		}
		
		// ---
		tick++;
		if (tick == speed)
		{
			// receive command through UNIX socket
			s2 = accept(s, (struct sockaddr *)&remote, &t);	
			if (s2 > 0)
			{
				int i = recv(s2, sockinput, 25, 0);
				sockinput[i] = 0; // null-terminated string
				close(s2);
				
				cmd[0] = strtok(sockinput, "- \n");
				cmd[1] = strtok(NULL, "- \n");
				
				if (strcmp (cmd[0], "brightness") == 0) // set brightness
				{
					brightness = atoi(cmd[1]); // 1 is maximum, 2 is half-brightness, etc.
				}
				else if (strcmp (cmd[0], "program") == 0) // set mode
				{
					program = atoi(cmd[1]); // 0...PROGRAMS
					
					// restart program
					tock = 0;
					gpioValuePos = prg[program][tock][0];
					directionPos = prg[program][tock][1];
					gpioValueNeg = prg[program][tock][2];
					directionNeg = prg[program][tock][3];
				}
				else if (strcmp (cmd[0], "speed") == 0) // set speed
				{
					speed = atoi(cmd[1]); // 1 is the fastest
					if (speed > 100)
					{
						speed = 100;
					}
				}
				else if (strcmp (cmd[0], "off") == 0) // off
				{
					gpioSet(GPIO_OE, GPIO_OFF);
				}
				else if (strcmp (cmd[0], "on") == 0) // on
				{
					gpioSet(GPIO_OE, GPIO_ON);
				}
			}

			// ---
			tick = 0;
			count++;
			gpioValuePos += directionPos;
			gpioValueNeg += directionNeg;
			if ((gpioValuePos == 100) || (gpioValuePos == -1) || (gpioValueNeg == 100) || (gpioValueNeg == -1) || (count == 100))
			{
				tock++;
				if (tock == 4)
				{
					tock = 0;
				}
				gpioValuePos = prg[program][tock][0];
				directionPos = prg[program][tock][1];
				gpioValueNeg = prg[program][tock][2];
				directionNeg = prg[program][tock][3];
				count = 0;
			}
		}
	}
}

void cleanup(int sig)
{
    do_exit = 1;
}

// GPIO functions
int gpioSetup()
{
	if ((m_mfd = open("/dev/mem", O_RDWR) ) < 0)
	{
		return -1;
	}

	m_base_addr = (unsigned long*)mmap(NULL, GPIO_BLOCK, PROT_READ|PROT_WRITE, MAP_SHARED, m_mfd, GPIO_ADDR);

	close(m_mfd);

	if (m_base_addr == MAP_FAILED)
	{
		return -2;
	}

	return 0;
}

// Direction: 0 - input, 1 - output
// Some GPIOs can not be set as inputs, see datasheet
void gpioDirection(int gpio, int direction)
{
	unsigned long value = *(m_base_addr + 0);
	if (direction == 1)
	{
		value |= (1 << gpio);
		*(m_base_addr+0) = value;
	}
	else
	{
		value &= ~(1 << gpio);
		*(m_base_addr+0) = value;
	}
}

int gpioRead(int gpio)
{
	unsigned long value = *(m_base_addr + 1);
	return (value & (1 << gpio));
}

// Uses 2 registers: one to set GPIO and another to clear GPIO
void gpioSet(int gpio, int value)
{
	if (value == 0)
	{
		*(m_base_addr + 4) = (1 << gpio);
	}
	else
	{
		*(m_base_addr + 3) = (1 << gpio);
	}
}

////////////////////////////////////////////////////////////////////////////////

void nsleep(unsigned long nsecs)
{
	if(nsecs == 0)
	{
		nanosleep(NULL, NULL);
	}

	struct timespec ts;

	ts.tv_sec=nsecs / 1000000000L;
	ts.tv_nsec=nsecs % 1000000000L;

	nanosleep(&ts,NULL);
}

////////////////////////////////////////////////////////////////////////////////

const int nsleep0_factor=3000;
const int nsleep1_factor=70;

void _usleep(unsigned long usecs)
{
	if(usecs == 0)
	{
		return;
	}

	unsigned long value=(usecs*1000)/nsleep0_factor;

	if(value > nsleep1_factor)
	{
		nsleep((value-nsleep1_factor)*nsleep0_factor);
		value=value % nsleep1_factor;
	}

	for(unsigned long i=0;i < value;++i)
	{
		nsleep(0);
	}
}

////////////////////////////////////////////////////////////////////////////////

