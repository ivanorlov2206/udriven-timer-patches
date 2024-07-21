# Userspace-driven ALSA timers

There are multiple possible timer sources which could be useful for
the sound stream synchronization: `hrtimers`, hardware clocks (e.g. PTP),
timer wheels (`jiffies`). Currently, using one of them to synchronize
the audio stream of `snd-aloop` module would require writing a
kernel-space driver which exports an ALSA timer through the
`snd_timer` interface.

However, it is not really convenient for application writers, who may
want to define their custom timer sources in order to synchronize
substreams of `snd-aloop`. For instance, we could have a network
application which receives frames and sends them to `snd-aloop` pcm
device, and another application listening on the other end of `snd-aloop`.
It makes sense to transfer a new period of data only when certain amount
of bytes is received through the network, but definitely not when a
certain amount of jiffies on a local system elapses. Since all of the
devices are purely virtual it won't introduce any glitches and will
help the application developers to avoid using sample-rate conversion.

This patch introduces userspace-driven ALSA timers. The timer can be
created from the userspace using the new ioctl `SNDRV_TIMER_IOCTL_CREATE`.
After creating a timer, it becomes available for use system-wide, so
it can be passed to snd-aloop as a timer source (timer_source parameter
would be `-1.4.{timer_id}`). When the userspace app decides to trigger
a timer, it calls another ioctl `SNDRV_TIMER_IOCTL_TRIGGER` on the file
descriptor of a timer. It initiates a transfer of a new period of data.

I believe introducing new ioctl calls is quite inconvenient (as we have
a limited amount of them), but other possible ways of app <-> kernel
communication (like virtual FS) seem completely inappropriate for this
task (but I'd love to discuss alternative solutions).

Userspace-driven timers are associated with file descriptors. If the
application wishes to destroy the timer, it can simply close the file.

To sum up, the userspace-driven ALSA timers allow us to create virtual
timers which can be used for `snd-aloop` synchronization.

## Patch list

* `0001-ALSA-aloop-Allow-using-global-timers.patch` - enables usage of global timers for the snd-aloop module.
To use a global timer, simply pass `-1` as a device id when setting `timer_source` module parameter.
* `0002-ALSA-timer-Introduce-userspace-driven-timers.patch` - introduces the support for userspace-driven ALSA
timers.

## Test application

First of all, make sure that both patches are applied to your kernel. To do that, copy both patches to the kernel
tree and run the following commands:
```
$ git am < 0001-ALSA-aloop-Allow-using-global-timers.patch
$ git am < 0002-ALSA-timer-Introduce-userspace-driven-timers.patch
```

Set the `INSTALL_HDR_PATH` environment variable to point to the directory you'd like to install the headers to:
```bash
$ export INSTALL_HDR_PATH=...
```

After that, build your kernel and install the kernel headers with `make headers_install` (it is highly recommended
to do it on VM or using [virtme-ng](https://github.com/arighi/virtme-ng))

The [test application](/test-app/test.c) which demonstrates the usage of userspace-driven ALSA timers is located
in the `test-app` folder of this repo. To build the app, run


```bash
$ cd test-app
$ make
```

To run this app properly, make sure that `snd-aloop` module is not currently loaded (since the app will try
to load it automatically), and run the following command as sudo:
```
# ./test
```

If the execution is successful, you are going to see a lot of `ABCD` on the screen.
