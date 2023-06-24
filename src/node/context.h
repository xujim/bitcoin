// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CONTEXT_H
#define BITCOIN_NODE_CONTEXT_H

#include <kernel/context.h>

#include <cassert>
#include <functional>
#include <memory>
#include <vector>

class ArgsManager;
class BanMan;
class AddrMan;
class CBlockPolicyEstimator;
class CConnman;
class CScheduler;
class CTxMemPool;
class ChainstateManager;
class NetGroupManager;
class PeerManager;
namespace interfaces {
class Chain;
class ChainClient;
class Init;
class WalletLoader;
} // namespace interfaces

namespace node {
class KernelNotifications;

//! NodeContext struct containing references to chain state and connection
//! state.
//! 更多是对一些全局对象的封装，如此避免这些全局变量过于分散。
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
struct NodeContext {
    //! libbitcoin_kernel context
    std::unique_ptr<kernel::Context> kernel;
    //! Init interface for initializing current process and connecting to other processes.
    interfaces::Init* init{nullptr};
    //! 地址管理
    std::unique_ptr<AddrMan> addrman; 
    //! 链接管理
    std::unique_ptr<CConnman> connman; 
    //! tx的缓存池
    std::unique_ptr<CTxMemPool> mempool;
    //! 网络组
    std::unique_ptr<const NetGroupManager> netgroupman;
    //! 根据policy统计fee
    std::unique_ptr<CBlockPolicyEstimator> fee_estimator;
    //! 对等节点管理
    std::unique_ptr<PeerManager> peerman;
    //! 链状态管理
    std::unique_ptr<ChainstateManager> chainman;
    //! 封禁节点的管理
    std::unique_ptr<BanMan> banman;
    ArgsManager* args{nullptr}; // Currently a raw pointer because the memory is not managed by this struct
    //! 当前链
    std::unique_ptr<interfaces::Chain> chain;
    //! List of all chain clients (wallet processes or other client) connected to node.
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;
    //! Reference to chain client that should used to load or create wallets
    //! opened by the gui.
    interfaces::WalletLoader* wallet_loader{nullptr};
    std::unique_ptr<CScheduler> scheduler;
    std::function<void()> rpc_interruption_point = [] {};
    std::unique_ptr<KernelNotifications> notifications;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the NodeContext struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    NodeContext();
    ~NodeContext();
};
} // namespace node

#endif // BITCOIN_NODE_CONTEXT_H
