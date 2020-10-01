#include <nlink/work.h>
#include <errno.h>

static void
nlink_win_xtract_work(struct nlink_work *work)
{
	work->state = NLINK_DANGLING_WORK_STATE;

	dlist_remove(&work->node);
#if defined(CONFIG_NLINK_ASSERT)
	dlist_init(&work->node);
#endif /* defined(CONFIG_NLINK_ASSERT) */
}

struct nlink_work *
nlink_win_acquire_work(struct nlink_win *win)
{
	nlink_win_assert(win);

	struct nlink_work *work = NULL;

	if (!dlist_empty(&win->free)) {
		work = dlist_entry(dlist_first(&win->free),
		                   struct nlink_work,
		                   node);
		nlink_assert(work->state == NLINK_FREE_WORK_STATE);

		nlink_win_xtract_work(work);
	}

	return work;
}

void
nlink_win_release_work(struct nlink_win *win, struct nlink_work *work)
{
	nlink_win_assert(win);
	nlink_assert(work);
	nlink_assert(dlist_empty(&work->node));
	nlink_assert(work->state == NLINK_DANGLING_WORK_STATE);

	work->state = NLINK_FREE_WORK_STATE;

	dlist_nqueue_front(&win->free, &work->node);
}

void
nlink_win_sched_work(struct nlink_win  *win,
                     struct nlink_work *work,
                     uint32_t           seqno)
{
	nlink_win_assert(win);
	nlink_assert(win->cnt < win->nr);
	nlink_assert(work);
	nlink_assert(work->state == NLINK_DANGLING_WORK_STATE);
	nlink_assert(dlist_empty(&work->node));

	work->state = NLINK_PENDING_WORK_STATE;
	work->seqno = seqno;

	win->cnt++;
	dlist_nqueue_back(&win->pend[seqno % win->nr], &work->node);
}

struct nlink_work *
nlink_win_pull_work(struct nlink_win *win, uint32_t seqno)
{
	nlink_win_assert(win);
	nlink_assert(win->cnt);

	struct nlink_work *work;

	dlist_foreach_entry(&win->pend[seqno % win->nr], work, node) {
		nlink_assert(work->state == NLINK_PENDING_WORK_STATE);

		if (work->seqno == seqno)
			goto found;
	}

	return NULL;

found:
	nlink_win_xtract_work(work);
	win->cnt--;

	return work;
}

bool
nlink_win_cancel_work(struct nlink_win *win, struct nlink_work *work)
{
	nlink_win_assert(win);
	nlink_assert(work);
	nlink_assert(work->state != NLINK_FREE_WORK_STATE);

	if (work->state == NLINK_PENDING_WORK_STATE) {
		nlink_assert(win->cnt);
		nlink_assert(!dlist_empty(&work->node));

		nlink_win_xtract_work(work);
		win->cnt--;

		return true;
	}

	return false;
}

struct nlink_work *
nlink_win_drain_work(struct nlink_win *win, unsigned int *slot)
{
	nlink_win_assert(win);
	nlink_assert(slot);
	nlink_assert(*slot <= win->nr);

	unsigned int       s;
	struct nlink_work *work = NULL;

	if (win->cnt) {
		for (s = *slot; s < win->nr; s++) {
			if (!dlist_empty(&win->pend[s]))
				break;
		}

		if (s < win->nr) {
			work = dlist_entry(dlist_dqueue_front(&win->pend[s]),
					   struct nlink_work,
					   node);
			nlink_assert(work->state == NLINK_PENDING_WORK_STATE);

			nlink_win_xtract_work(work);
			win->cnt--;
		}

		*slot = s;
	}

	return work;
}

void
nlink_win_register_work(struct nlink_win *win, struct nlink_work *work)
{
	nlink_win_assert(win);
	nlink_assert(!win->cnt);
	nlink_assert(work);

	work->state = NLINK_FREE_WORK_STATE;
	dlist_nqueue_back(&win->free, &work->node);
}

int
nlink_win_init(struct nlink_win *win, unsigned int nr)
{
	nlink_assert(win);
	nlink_assert(nr);

	unsigned int w;

	win->pend = malloc(nr * sizeof(win->pend[0]));
	if (!win->pend)
		return -errno;

	dlist_init(&win->free);
	for (w = 0; w < nr; w++)
		dlist_init(&win->pend[w]);

	win->cnt = 0;
	win->nr = nr;

	return 0;
}

void
nlink_win_fini(const struct nlink_win *win)
{
	nlink_win_assert(win);
	nlink_assert(!win->cnt);
#if defined(CONFIG_NLINK_ASSERT)
	{
		unsigned int       w;
		struct nlink_work *work;

		for (w = 0; w < win->nr; w++)
			nlink_assert(dlist_empty(&win->pend[w]));

		w = 0;
		dlist_foreach_entry(&win->free, work, node)
			w++;
		nlink_assert(w == win->nr);
	}
#endif /* defined(CONFIG_NLINK_ASSERT) */

	free(win->pend);
}
