/* Copyright (c) 2023 Renmin University of China
RMDB is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
        http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "lock_manager.h"

/**
 * @description: 申请行级共享锁
 * @return {bool} 加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {Rid&} rid 加锁的目标记录ID 记录所在的表的fd
 * @param {int} tab_fd
 */
bool LockManager::lock_shared_on_record(Transaction* txn, const Rid& rid, int tab_fd) {
    // Todo:
    // 1. 通过mutex申请访问全局锁表
    // 2. 检查事务的状态
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    // 提示：步骤5中的阻塞操作可以通过条件变量来完成，所有加锁操作都遵循上述步骤，在下面的加锁操作中不再进行注释提示

    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, rid, LockDataType::RECORD);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            // 已经有锁了，加锁成功
            return true;
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有写锁no wait
    if (lock_request_queue->group_lock_mode_ == GroupLockMode::IX ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::X ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::SIX) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::SHARED};
    lock_request.granted_ = true;
    lock_request_queue->group_lock_mode_ = GroupLockMode::S;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    lock_request_queue->shared_lock_num_++;
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 申请行级排他锁
 * @return {bool} 加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {Rid&} rid 加锁的目标记录ID
 * @param {int} tab_fd 记录所在的表的fd
 */
bool LockManager::lock_exclusive_on_record(Transaction* txn, const Rid& rid, int tab_fd) {
    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, rid, LockDataType::RECORD);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            if (iter.lock_mode_ == LockMode::EXLUCSIVE) {
                // 已经有写锁了，加锁成功
                return true;
            } else if (iter.lock_mode_ == LockMode::SHARED && lock_request_queue->request_queue_.size() == 1) {
                // 已经有读锁了，且只有当前事务在读，升级为写锁
                iter.lock_mode_ = LockMode::EXLUCSIVE;
                lock_request_queue->group_lock_mode_ = GroupLockMode::X;
                return true;
            } else {
                throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
            }
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有锁no wait
    if (lock_request_queue->group_lock_mode_ != GroupLockMode::NON_LOCK) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::EXLUCSIVE};
    lock_request_queue->group_lock_mode_ = GroupLockMode::X;
    lock_request.granted_ = true;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 申请表级读锁
 * @return {bool} 返回加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {int} tab_fd 目标表的fd
 */
bool LockManager::lock_shared_on_table(Transaction* txn, int tab_fd) {
    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, LockDataType::TABLE);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            if (iter.lock_mode_ == LockMode::SHARED || iter.lock_mode_ == LockMode::EXLUCSIVE ||
                iter.lock_mode_ == LockMode::S_IX) {
                // 已经有高于S的锁了，加锁成功
                return true;
            } else if ((iter.lock_mode_ == LockMode::INTENTION_SHARED) &&
                       (lock_request_queue->group_lock_mode_ == GroupLockMode::S ||
                        lock_request_queue->group_lock_mode_ == GroupLockMode::IS)) {
                // 已经有IS锁了，且其他事务不持有写锁，升级为S锁
                iter.lock_mode_ = LockMode::SHARED;
                lock_request_queue->group_lock_mode_ = GroupLockMode::S;
                lock_request_queue->shared_lock_num_++;
                return true;
            } else if (iter.lock_mode_ == LockMode::INTENTION_EXCLUSIVE && lock_request_queue->IX_lock_num_ == 1) {
                // 已经有IX锁了，且其他事务不持有IX锁，升级为SIX锁
                iter.lock_mode_ = LockMode::S_IX;
                lock_request_queue->group_lock_mode_ = GroupLockMode::SIX;
                lock_request_queue->shared_lock_num_++;
                return true;
            } else {
                throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
            }
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有写锁no wait
    if (lock_request_queue->group_lock_mode_ == GroupLockMode::IX ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::X ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::SIX) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::SHARED};
    lock_request_queue->group_lock_mode_ = GroupLockMode::S;
    lock_request.granted_ = true;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    lock_request_queue->shared_lock_num_++;
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 申请表级写锁
 * @return {bool} 返回加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {int} tab_fd 目标表的fd
 */
bool LockManager::lock_exclusive_on_table(Transaction* txn, int tab_fd) {
    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, LockDataType::TABLE);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            if (iter.lock_mode_ == LockMode::EXLUCSIVE) {
                // 已经有写锁了，加锁成功
                return true;
            } else if (lock_request_queue->request_queue_.size() == 1) {
                // 只有当前事务与此数据相关，才能升级
                iter.lock_mode_ = LockMode::EXLUCSIVE;
                lock_request_queue->group_lock_mode_ = GroupLockMode::X;
                return true;
            } else {
                throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
            }
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有锁no wait
    if (lock_request_queue->group_lock_mode_ != GroupLockMode::NON_LOCK) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::EXLUCSIVE};
    lock_request_queue->group_lock_mode_ = GroupLockMode::X;
    lock_request.granted_ = true;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 申请表级意向读锁
 * @return {bool} 返回加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {int} tab_fd 目标表的fd
 */
bool LockManager::lock_IS_on_table(Transaction* txn, int tab_fd) {
    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, LockDataType::TABLE);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            // 已经有锁了，加锁成功
            return true;
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有写锁no wait
    if (lock_request_queue->group_lock_mode_ == GroupLockMode::X) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    } else if (lock_request_queue->group_lock_mode_ == GroupLockMode::NON_LOCK) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::IS;
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::INTENTION_SHARED};
    lock_request.granted_ = true;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 申请表级意向写锁
 * @return {bool} 返回加锁是否成功
 * @param {Transaction*} txn 要申请锁的事务对象指针
 * @param {int} tab_fd 目标表的fd
 */
bool LockManager::lock_IX_on_table(Transaction* txn, int tab_fd) {
    // 1. 通过mutex申请访问全局锁表
    std::unique_lock<std::mutex> lock{latch_};
    // 2. 检查事务的状态
    // DEFAULT, GROWING, SHRINKING, COMMITTED, ABORTED
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 开启两阶段封锁协议的封锁阶段
        txn->set_state(TransactionState::GROWING);
    } else if (txn->get_state() == TransactionState::GROWING) {
        // pass
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::LOCK_ON_SHIRINKING);
        return false;
    }
    // 3. 查找当前事务是否已经申请了目标数据项上的锁，如果存在则根据锁类型进行操作，否则执行下一步操作
    LockDataId lock_data_id(tab_fd, LockDataType::TABLE);
    if (lock_table_.count(lock_data_id) == 0) {
        lock_table_.emplace(std::piecewise_construct, std::forward_as_tuple(lock_data_id), std::forward_as_tuple());
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    for (auto& iter : lock_request_queue->request_queue_) {
        if (iter.txn_id_ == txn->get_transaction_id()) {
            if (iter.lock_mode_ == LockMode::INTENTION_EXCLUSIVE || iter.lock_mode_ == LockMode::EXLUCSIVE ||
                iter.lock_mode_ == LockMode::S_IX) {
                // 已经有写锁了，加锁成功
                return true;
            } else if ((iter.lock_mode_ == LockMode::INTENTION_SHARED) &&
                       (lock_request_queue->group_lock_mode_ == GroupLockMode::IS ||
                        lock_request_queue->group_lock_mode_ == GroupLockMode::IX)) {
                iter.lock_mode_ = LockMode::INTENTION_EXCLUSIVE;
                lock_request_queue->group_lock_mode_ = GroupLockMode::IX;
                lock_request_queue->IX_lock_num_++;
                return true;
            } else if (iter.lock_mode_ == LockMode::SHARED && lock_request_queue->shared_lock_num_ == 1) {
                // 只有当前事务与此数据相关，才能升级
                iter.lock_mode_ = LockMode::S_IX;
                lock_request_queue->group_lock_mode_ = GroupLockMode::SIX;
                lock_request_queue->IX_lock_num_++;
                return true;
            } else {
                throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
            }
        }
    }
    // 4. 将要申请的锁放入到全局锁表中，并通过组模式来判断是否可以成功授予锁
    // NON_LOCK, IS, IX, S, X, SIX
    // 有锁no wait
    if (lock_request_queue->group_lock_mode_ == GroupLockMode::S ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::X ||
        lock_request_queue->group_lock_mode_ == GroupLockMode::SIX) {
        throw TransactionAbortException(txn->get_transaction_id(), AbortReason::DEADLOCK_PREVENTION);
    }
    // 5. 如果成功，更新目标数据项在全局锁表中的信息，否则阻塞当前操作
    LockRequest lock_request = LockRequest{txn->get_transaction_id(), LockMode::INTENTION_EXCLUSIVE};
    lock_request_queue->group_lock_mode_ = GroupLockMode::IX;
    lock_request_queue->IX_lock_num_++;
    lock_request.granted_ = true;
    lock_request_queue->request_queue_.emplace_back(lock_request);
    txn->get_lock_set()->emplace(lock_data_id);
    return true;
}

/**
 * @description: 释放锁
 * @return {bool} 返回解锁是否成功
 * @param {Transaction*} txn 要释放锁的事务对象指针
 * @param {LockDataId} lock_data_id 要释放的锁ID
 */
bool LockManager::unlock(Transaction* txn, LockDataId lock_data_id) {
    std::unique_lock<std::mutex> lock(latch_);
    if (txn->get_state() == TransactionState::DEFAULT) {
        // 封锁阶段没有获得锁 pass
    } else if (txn->get_state() == TransactionState::GROWING) {
        // 开启两阶段封锁协议的放锁阶段
        txn->set_state(TransactionState::SHRINKING);
    } else if (txn->get_state() == TransactionState::COMMITTED) {
        return false;
    } else if (txn->get_state() == TransactionState::ABORTED) {
        return false;
    } else if (txn->get_state() == TransactionState::SHRINKING) {
        // pass
    }
    if (lock_table_.count(lock_data_id) == 0) {
        return true;
    }
    LockRequestQueue* lock_request_queue = &lock_table_[lock_data_id];
    auto it = lock_request_queue->request_queue_.begin();
    for (; it != lock_table_[lock_data_id].request_queue_.end(); it++) {
        if (it->txn_id_ == txn->get_transaction_id()) break;
    }
    if (it == lock_request_queue->request_queue_.end()) {
        return true;
    }
    switch (it->lock_mode_) {
        case LockMode::SHARED:
            lock_request_queue->shared_lock_num_--;
            break;
        case LockMode::INTENTION_EXCLUSIVE:
            lock_request_queue->IX_lock_num_--;
            break;
        case LockMode::S_IX:
            lock_request_queue->shared_lock_num_--;
            lock_request_queue->IX_lock_num_--;
            break;
    }
    lock_request_queue->request_queue_.erase(it);
    // 确定queue中级别最大的锁，赋值group_lock_mode_
    int S_num = 0;
    int X_num = 0;
    int IS_num = 0;
    int IX_num = 0;
    int SIX_num = 0;
    for (auto& iter : lock_request_queue->request_queue_) {
        switch (iter.lock_mode_) {
            case LockMode::SHARED:
                S_num++;
                break;
            case LockMode::EXLUCSIVE:
                X_num++;
                break;
            case LockMode::INTENTION_SHARED:
                IS_num++;
                break;
            case LockMode::INTENTION_EXCLUSIVE:
                IX_num++;
                break;
            case LockMode::S_IX:
                SIX_num++;
                break;
        }
    }
    if (X_num > 0) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::X;
    } else if (SIX_num > 0) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::SIX;
    } else if (S_num > 0) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::S;
    } else if (IX_num > 0) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::IX;
    } else if (IS_num > 0) {
        lock_request_queue->group_lock_mode_ = GroupLockMode::IS;
    } else {
        lock_request_queue->group_lock_mode_ = GroupLockMode::NON_LOCK;
    }
    return true;
}