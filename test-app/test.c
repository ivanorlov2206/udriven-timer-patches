#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <pthread.h>
#include <sound/asound.h>

/*
 * Userspace timer definition. We have to provide the desired rate and period size (in frames)
 * to convince snd-aloop that our timer works correctly and has right HW parameters
 */
static struct snd_userspace_timer timer = {
	.rate = 8000,
	.period = 4410,
	/* id can be set to anything here, it is going to be overwritten by timer creation ioctl */
	.id = -1,
};
static int timer_fd;

static int create_timer(void)
{
	int timer_i_fd;

	timer_i_fd = open("/dev/snd/timer", O_RDWR | O_CLOEXEC);
	if (timer_i_fd < 0) {
		perror("Can't open /dev/snd/timer");
		return -1;
	}

	timer_fd = ioctl(timer_i_fd, SNDRV_TIMER_IOCTL_CREATE, &timer);
	if (timer_fd < 0) {
		fprintf(stderr, "Can't create the timer: %d\n", timer_fd);
		return -1;
	}

	/* Now we have timer id and we can use it for snd-aloop */
	printf("Timer id: %d\n", timer.id);

	return 0;
}

static void load_snd_aloop(void)
{
	char command[64];

	/*
	 * The timer_source would always start with "-1.4." since we are using global timer with
	 * device number = 4 and subdevice number = <timer_id>
	 */
	sprintf(command, "modprobe snd-aloop timer_source=\"-1.4.%d\"", timer.id);
	system(command);
}

#define TIMER_FREQ_SEC 1
static void *ticking_thread(void *data)
{
	int i;

	for (i = 0; i < 10; i++) {
		/* Well, fire the timer! */
		ioctl(timer_fd, SNDRV_TIMER_IOCTL_FIRE, NULL);
		sleep(TIMER_FREQ_SEC);
	}
}

#define BUF_SIZE 2048
static char buf[BUF_SIZE];
static void record_play(void)
{
	pthread_t tt;

	/* Starting recording and playing at the same time on snd-aloop devices */
	FILE *rfp = popen("./record.sh", "r");
	pthread_create(&tt, NULL, ticking_thread, NULL);
	while (read(fileno(rfp), buf, BUF_SIZE) != 0) {
		buf[BUF_SIZE - 1] = 0;
		printf("Some data: %s\n", buf);
	}
	pclose(rfp);
	pthread_join(tt, NULL);
}

static void unload(void)
{
	system("rmmod snd-aloop");
}

int main(void)
{
	int ret;

	ret = create_timer();
	if (ret != 0)
		return EXIT_FAILURE;

	load_snd_aloop();

	record_play();

	close(timer_fd);
	return EXIT_SUCCESS;
}
