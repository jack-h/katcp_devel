
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <linux/input.h>

#include <katcp.h>

#include "tcpborphserver3.h"

int open_evdev_tbs(struct katcp_dispatch *d, char *name)
{
#define BUFFER 64
  int i, fd;
  char buffer[BUFFER];

  for(i = 0; ;i++){
    snprintf(buffer, BUFFER - 1, "/dev/input/event%d", i);
    buffer[BUFFER - 1] = '\0';

    fd = open(buffer, O_RDWR | O_CLOEXEC);
    if(fd < 0){
      switch(errno){
        case ENODEV : 
          log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no device driver backing %s, giving up", buffer);
          return -1;
        case ENOENT : 
          log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, (i == 0) ? "unable to locate device file %s, try creating it" : "no further device file at %s to match %s, giving up", buffer, name);
          return -1;
        default : 
          log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "skipping device %s, error given: %s", buffer, strerror(errno));
          break;
      }
    }

    if(ioctl(fd, EVIOCGNAME(BUFFER - 1), buffer) < 0){
      log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "unable to acquire name for %s: %s", buffer, strerror(errno));
      close(fd);
      return 1;
    }

    buffer[BUFFER - 1] = '\0';

    if(strcmp(buffer, name)){
      log_message_katcp(d, KATCP_LEVEL_DEBUG, NULL, "input device name %s does not match request %s", buffer, name);
      close(fd);
      continue;
    }

    log_message_katcp(d, KATCP_LEVEL_DEBUG, NULL, "found matching event%d for name %s", i, name);
    return fd;
  }
#undef BUFFER 
}

int run_chasis_tbs(struct katcp_dispatch *d, struct katcp_arb *a, unsigned int mode)
{
  struct input_event event;
  int rr, fd;

  fd = fileno_arb_katcp(d, a);

  if(mode & KATCP_ARB_READ){
    rr = read(fd, &event, sizeof(struct input_event));
    if(rr == sizeof(struct input_event)){
      log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "input event type=%d, code=%d, value=%d", event.type, event.code, event.value);
      /* TODO: one of: shut down server, emit message, invoke callback */
    }
  }

  return 0;
}

struct katcp_arb *chasis_init_tbs(struct katcp_dispatch *d, char *name)
{
  struct katcp_arb *a;
  int fd;

  a = find_arb_katcp(d, name);
  if(a){
    log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "logic for %s already initialised", name);
    return a;
  }

  fd = open_evdev_tbs(d, name);
  if(fd < 0){
    return NULL;
  }

  a = create_arb_katcp(d, name, fd, KATCP_ARB_READ, &run_chasis_tbs, NULL);
  if(a == NULL){
    close(fd);
    return NULL;
  }

  return a;
}

int start_chasis_cmd(struct katcp_dispatch *d, int argc)
{
  char *match;
  struct tbs_raw *tr;

  tr = get_current_mode_katcp(d);
  if(tr == NULL){
    log_message_katcp(d, KATCP_LEVEL_FATAL, NULL, "unable to get raw state");
    return KATCP_RESULT_FAIL;
  }

  if(tr->r_chasis){
    log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "chasis logic already registered");
    return KATCP_RESULT_FAIL;
  }

  if(argc <= 1){
    match = "roach";
  } else {
    match = arg_string_katcp(d, 1);
    if(match == NULL){
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "unable to acquire valid match");
      return KATCP_RESULT_FAIL;
    }
  }

  tr->r_chasis = chasis_init_tbs(d, match);
  if(tr->r_chasis == NULL){
    return KATCP_RESULT_FAIL;
  }

  return KATCP_RESULT_OK;
}

int write_event_tbs(struct katcp_dispatch *d, struct katcp_arb *a, int type, int code, int value)
{
  int fd, result;
  struct input_event event;

  fd = fileno_arb_katcp(d, a);
  if(fd < 0){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no valid file descriptor as event target");
    return -1;
  }

  event.type  = type;
  event.code  = code;
  event.value = value;

  result = write(fd, &event, sizeof(struct input_event));
  if(result == sizeof(struct input_event)){
    return 0;
  }

  if(result < 0){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "event write failed: %s", strerror(errno));
  } else {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "logic problem, short event write");
  }

  return -1;
}

int led_chasis_cmd(struct katcp_dispatch *d, int argc)
{
  struct tbs_raw *tr;
  int code, value;
  char *name, *parm;

  if(argc <= 1){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "insufficient parameters");
    return KATCP_RESULT_FAIL;
  }

  tr = get_current_mode_katcp(d);
  if(tr == NULL){
    log_message_katcp(d, KATCP_LEVEL_FATAL, NULL, "unable to get raw state");
    return KATCP_RESULT_FAIL;
  }

  if(tr->r_chasis == NULL){
    log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "no chasis logic registered");
    return KATCP_RESULT_FAIL;
  }

  name = arg_string_katcp(d, 1);
  if(name == NULL){
    log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "unable to acquire led name");
    return KATCP_RESULT_FAIL;
  }

  /* this should be an array lookup, but it is only two leds */
  if(!strcmp(name, "red")){
    code = LED_SUSPEND;
  } else if(!strcmp(name, "green")){
    code = LED_MISC;
  } else {
    log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "unsupported led %s", name);
    return KATCP_RESULT_FAIL;
  }

  value = 0;
  parm = arg_string_katcp(d, 2);
  if(parm){
    if(strcmp(parm, "off") && 
       strcmp(parm, "false") && 
       strcmp(parm, "0")
      ){
      value = 1;
    }
  }

  if(write_event_tbs(d, tr->r_chasis, EV_LED, code, value) < 0){
    return KATCP_RESULT_FAIL;
  }

  if(write_event_tbs(d, tr->r_chasis, EV_SYN, 0, 0) < 0){
    return KATCP_RESULT_FAIL;
  }

  return KATCP_RESULT_OK;
}

