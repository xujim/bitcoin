// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETGROUP_H
#define BITCOIN_NETGROUP_H

#include <netaddress.h>
#include <uint256.h>

#include <vector>

/**
 * Netgroup manager
 */
class NetGroupManager {
public:
    explicit NetGroupManager(std::vector<bool> asmap)
        : m_asmap{std::move(asmap)}
    {}

    /** Get a checksum identifying the asmap being used. */
    uint256 GetAsmapChecksum() const;

    /**
     * Get the canonical identifier of the network group for address.
     *
     * The groups are assigned in a way where it should be costly for an attacker to
     * obtain addresses with many different group identifiers, even if it is cheap
     * to obtain addresses with the same identifier.
     *
     * @note No two connections will be attempted to addresses with the same network
     *       group.
     */
    std::vector<unsigned char> GetGroup(const CNetAddr& address) const;

    /**
     *  Get the autonomous system on the BGP path to address.
     * !TODO: 搞不明白什么是asmap或者mappedas以及BGP。tongyi说是：MappedAS是比特币的一种管理机制，
     * 用于将区块链上的交易与现实世界中的商业交易进行映射。在比特币的系统中，每个区块都包含了多个交易记录，
     * 而每个区块的创建者（即矿工）都可以为这个区块的交易记录设置一个对应的标识符（即MappedAS），
     * 这个标识符可以用于将区块链上的交易与现实世界中的商业交易进行映射。例如，一个交易可能被映射为一个公司的销售记录，
     * 而这个销售记录可能被映射为一个银行的交易记录。通过使用MappedAS，比特币的区块链可以与现实世界中的不同系统和机构进行连接和交互，
     * 从而实现更加复杂和全面的交易记录管理。
     *  The ip->AS mapping depends on how asmap is constructed.
     */
    uint32_t GetMappedAS(const CNetAddr& address) const;

private:
    /** Compressed IP->ASN mapping, loaded from a file when a node starts.
     *
     * This mapping is then used for bucketing nodes in Addrman and for
     * ensuring we connect to a diverse set of peers in Connman. The map is
     * empty if no file was provided.
     *
     * If asmap is provided, nodes will be bucketed by AS they belong to, in
     * order to make impossible for a node to connect to several nodes hosted
     * in a single AS. This is done in response to Erebus attack（一种比特币网络攻击方式，参考：https://www.anquanke.com/post/id/199461）, 
     * but also to generally diversify the connections every node creates, especially
     * useful when a large fraction of nodes operate under a couple of cloud
     * providers.
     *
     * If a new asmap is provided, the existing addrman records are
     * re-bucketed.
     *
     * This is initialized in the constructor, const, and therefore is
     * thread-safe. */
    const std::vector<bool> m_asmap;
};

#endif // BITCOIN_NETGROUP_H
