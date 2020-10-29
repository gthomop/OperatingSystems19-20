/***** record.h *****/
#ifndef _RECORD_H
#define _RECORD_H

#define SIZEofBUFF 20
#define SSizeofBUFF 6

typedef struct {
  long custid;
  char FirstName[SIZEofBUFF];
  char LastName[SIZEofBUFF];
  char Street[SIZEofBUFF];
  int HouseID;
  char City[SIZEofBUFF];
  char postcode[SSizeofBUFF];
  float amount;
} Record;

/* macros for columnid */
#define CUSTID 1
#define FIRSTNAME 2
#define LASTNAME 3
#define STREET 4
#define HOUSEID 5
#define CITY 6
#define POSTCODE 7
#define AMOUNT 8

#endif