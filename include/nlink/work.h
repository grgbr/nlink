#ifndef _NLINK_WORK_H
#define _NLINK_WORK_H

#include <nlink/nlink.h>
#include <utils/dlist.h>
#include <sys/types.h>

struct nlink_work;

enum nlink_work_state  {
	NLINK_FREE_WORK_STATE,
	NLINK_PENDING_WORK_STATE,
	NLINK_DANGLING_WORK_STATE,
};

struct nlink_work {
	struct dlist_node      node;
	enum nlink_work_state  state;
	uint32_t               seqno;
};

struct nlink_win {
	unsigned int       cnt;
	unsigned int       nr;
	struct dlist_node *pend;
	struct dlist_node  free;
};

#define nlink_win_assert(_win) \
	nlink_assert(_win); \
	nlink_assert((_win)->nr); \
	nlink_assert((_win)->cnt <= (_win)->nr); \
	nlink_assert((_win)->pend)

static inline bool
nlink_win_has_work(const struct nlink_win *win)
{
	nlink_win_assert(win);

	return !!win->cnt;
}

extern struct nlink_work *
nlink_win_acquire_work(struct nlink_win *win);

extern void
nlink_win_release_work(struct nlink_win *win, struct nlink_work *work);

extern void
nlink_win_sched_work(struct nlink_win  *win,
                     struct nlink_work *work,
                     uint32_t           seqno);

static inline void
nlink_win_resched_work(struct nlink_win *win, struct nlink_work *work)
{
	nlink_win_sched_work(win, work, work->seqno);
}

extern struct nlink_work *
nlink_win_pull_work(struct nlink_win *win, uint32_t seqno);

extern bool
nlink_win_cancel_work(struct nlink_win *win, struct nlink_work *work);

extern struct nlink_work *
nlink_win_drain_work(struct nlink_win *win, unsigned int *slot);

extern void
nlink_win_register_work(struct nlink_win *win, struct nlink_work *work);

extern int
nlink_win_init(struct nlink_win *win, unsigned int nr);

extern void
nlink_win_fini(const struct nlink_win *win);

#endif /* _NLINK_WORK_H */
