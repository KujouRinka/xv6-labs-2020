#ifndef __KERNEL_DATE_H__
#define __KERNEL_DATE_H__

struct rtcdate {
  uint second;
  uint minute;
  uint hour;
  uint day;
  uint month;
  uint year;
};

#endif
