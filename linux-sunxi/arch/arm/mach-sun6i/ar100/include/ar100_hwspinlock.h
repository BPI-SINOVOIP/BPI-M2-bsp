/*
 *  arch/arm/mach-sun6i/ar100/include/ar100_hwspinlock.h
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __AR100_HW_SPINLOCK_H
#define __AR100_HW_SPINLOCK_H

/* the max number of hardware spinlock */
#define AR100_HW_SPINLOCK_NUM       (32)

/**
 * initialize hwspinlock.
 * @para:  none.
 *
 * returns:  OK if initialize hwspinlock succeeded, others if failed.
 */
int ar100_hwspinlock_init(void);

/**
 * exit hwspinlock.
 * @para:  none.
 *
 * returns:  OK if exit hwspinlock succeeded, others if failed.
 */
int ar100_hwspinlock_exit(void);

/**
 * lock an hwspinlock with timeout limit.
 * @hwid : an hwspinlock id which we want to lock.
 *
 * returns:  OK if lock hwspinlock succeeded, other if failed.
 */
int ar100_hwspin_lock_timeout(int hwid, unsigned int timeout);

/**
 * unlock a specific hwspinlock.
 * @hwid : an hwspinlock id which we want to unlock.
 *
 * returns:  OK if unlock hwspinlock succeeded, other if failed.
 */
int ar100_hwspin_unlock(int hwid);

int ar100_hwspinlock_standby_suspend(void);
int ar100_hwspinlock_standby_resume(void);

#endif  /* __AR100_HW_SPINLOCK_H */
