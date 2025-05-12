#ifndef _ARENA_FORMS_H_
#define _ARENA_FORMS_H_

extern char *WWWEncode(char *);
extern void SubmitForm(Form *, int, char *, int, int,  char *, int, char *, int, Image *, int, int, char *);
extern Form *GetForm(int, char *, int);
extern Form *FindForm(Form *,char *);

#endif /* _ARENA_FORMS_H_ */
