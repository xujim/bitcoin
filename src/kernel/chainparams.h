// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAINPARAMS_H
#define BITCOIN_KERNEL_CHAINPARAMS_H

#include <consensus/params.h>
#include <netaddress.h>
#include <primitives/block.h>
#include <protocol.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/hash_type.h>

#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

typedef std::map<int, uint256> MapCheckpoints;

struct CCheckpointData {
    MapCheckpoints mapCheckpoints;

    int GetHeight() const {
        const auto& final_checkpoint = mapCheckpoints.rbegin();
        return final_checkpoint->first /* height */;
    }
};

struct AssumeutxoHash : public BaseHash<uint256> {
    explicit AssumeutxoHash(const uint256& hash) : BaseHash(hash) {}
};

/**
 * 参考 https://github.com/jamesob/assumeutxo-docs/tree/2019-04-proposal/proposal
 * Holds configuration for use during UTXO snapshot load and validation. The contents
 * here are security critical, since they dictate which UTXO snapshots are recognized
 * as valid.
 * "assumeutxo"是一个比特币的术语，它指的是一种特定类型的交易。在比特币的交易系统中，每个区块都包含了一定数量的交易记录，这些交易记录被称为"未确认交易"（unconfirmed transactions）。
当一个人想要在比特币网络上进行一笔交易时，他们需要向网络发送一个交易请求，并支付一定的交易费用。这个交易请求被称为"猜测交易"（guess transaction），因为在发送交易请求时，交易者并不知道其他节点是否会接受这个请求。
如果其他节点接受了这个请求，那么这个交易就会被确认为"已确认交易"（confirmed transactions）。这时，这个交易就可以被记录在区块中，并且所有人都可以查看这个交易的历史记录。
而"assumeutxo"是指在没有确认交易的情况下进行的交易。在这种情况下，交易者发送了一个猜测交易，但是没有得到其他节点的确认，因此这笔交易并没有被记录在区块中。这意味着这笔交易没有被其他人所认可，交易的安全性和有效性无法得到保障。
因此，在进行比特币交易时，需要确保所有的交易都已经被确认，并且不存在"assumeutxo"的情况
 */
struct AssumeutxoData {
    //! The expected hash of the deserialized UTXO set.
    const AssumeutxoHash hash_serialized;

    //! Used to populate the nChainTx value, which is used during BlockManager::LoadBlockIndex().
    //!
    //! We need to hardcode the value here because this is computed cumulatively using block data,
    //! which we do not necessarily have at the time of snapshot load.
    const unsigned int nChainTx;
};

using MapAssumeutxo = std::map<int, const AssumeutxoData>;

/**
 * Holds various statistics on transactions within a chain. Used to estimate
 * verification progress during chain sync.
 *
 * See also: CChainParams::TxData, GuessVerificationProgress.
 */
struct ChainTxData {
    int64_t nTime;    //!< UNIX timestamp of last known number of transactions
    int64_t nTxCount; //!< total number of transactions between genesis and that timestamp
    double dTxRate;   //!< estimated number of transactions per second after that timestamp
};

/**
 * 一个比特币系统的参数信息，这些参数是可以后期配置修改的
 * CChainParams defines various tweakable parameters of a given instance of the
 * Bitcoin system.
 */
class CChainParams
{
public:
    enum Base58Type {
        PUBKEY_ADDRESS,
        SCRIPT_ADDRESS,
        SECRET_KEY,
        EXT_PUBLIC_KEY,
        EXT_SECRET_KEY,

        MAX_BASE58_TYPES
    };

    const Consensus::Params& GetConsensus() const { return consensus; }
    const CMessageHeader::MessageStartChars& MessageStart() const { return pchMessageStart; }
    uint16_t GetDefaultPort() const { return nDefaultPort; }
    uint16_t GetDefaultPort(Network net) const
    {
        return net == NET_I2P ? I2P_SAM31_PORT : GetDefaultPort();
    }
    uint16_t GetDefaultPort(const std::string& addr) const
    {
        CNetAddr a;
        return a.SetSpecial(addr) ? GetDefaultPort(a.GetNetwork()) : GetDefaultPort();
    }

    const CBlock& GenesisBlock() const { return genesis; }
    /** Default value for -checkmempool and -checkblockindex argument */
    bool DefaultConsistencyChecks() const { return fDefaultConsistencyChecks; }
    /** Policy: Filter transactions that do not match well-defined patterns */
    bool RequireStandard() const { return fRequireStandard; }
    /** If this chain is exclusively used for testing */
    bool IsTestChain() const { return m_is_test_chain; }
    /** If this chain allows time to be mocked */
    bool IsMockableChain() const { return m_is_mockable_chain; }
    uint64_t PruneAfterHeight() const { return nPruneAfterHeight; }
    /** Minimum free space (in GB) needed for data directory */
    uint64_t AssumedBlockchainSize() const { return m_assumed_blockchain_size; }
    /** Minimum free space (in GB) needed for data directory when pruned; Does not include prune target*/
    uint64_t AssumedChainStateSize() const { return m_assumed_chain_state_size; }
    /** Whether it is possible to mine blocks on demand (no retargeting) */
    bool MineBlocksOnDemand() const { return consensus.fPowNoRetargeting; }
    /** Return the chain type string */
    std::string GetChainTypeString() const { return ChainTypeToString(m_chain_type); }
    /** Return the chain type */
    ChainType GetChainType() const { return m_chain_type; }
    /** Return the list of hostnames to look up for DNS seeds */
    const std::vector<std::string>& DNSSeeds() const { return vSeeds; }
    const std::vector<unsigned char>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    const std::string& Bech32HRP() const { return bech32_hrp; }
    const std::vector<uint8_t>& FixedSeeds() const { return vFixedSeeds; }
    const CCheckpointData& Checkpoints() const { return checkpointData; }

    //! Get allowed assumeutxo configuration.
    //! @see ChainstateManager
    const MapAssumeutxo& Assumeutxo() const { return m_assumeutxo_data; }

    const ChainTxData& TxData() const { return chainTxData; }

    /**
     * SigNetOptions holds configurations for creating a signet CChainParams.
     */
    struct SigNetOptions {
        std::optional<std::vector<uint8_t>> challenge{};
        std::optional<std::vector<std::string>> seeds{};
    };

    /**
     * VersionBitsParameters holds activation parameters
     */
    struct VersionBitsParameters {
        int64_t start_time;
        int64_t timeout;
        int min_activation_height;
    };

    /**
     * RegTestOptions holds configurations for creating a regtest CChainParams.
     */
    struct RegTestOptions {
        std::unordered_map<Consensus::DeploymentPos, VersionBitsParameters> version_bits_parameters{};
        std::unordered_map<Consensus::BuriedDeployment, int> activation_heights{};
        bool fastprune{false};
    };

    static std::unique_ptr<const CChainParams> RegTest(const RegTestOptions& options);
    static std::unique_ptr<const CChainParams> SigNet(const SigNetOptions& options);
    static std::unique_ptr<const CChainParams> Main();
    static std::unique_ptr<const CChainParams> TestNet();

protected:
    CChainParams() {}

    Consensus::Params consensus;
    CMessageHeader::MessageStartChars pchMessageStart;
    uint16_t nDefaultPort;
    uint64_t nPruneAfterHeight;
    uint64_t m_assumed_blockchain_size;
    uint64_t m_assumed_chain_state_size;
    std::vector<std::string> vSeeds;
    std::vector<unsigned char> base58Prefixes[MAX_BASE58_TYPES];
    std::string bech32_hrp;
    ChainType m_chain_type;
    CBlock genesis;
    std::vector<uint8_t> vFixedSeeds;
    bool fDefaultConsistencyChecks;
    bool fRequireStandard;
    bool m_is_test_chain;
    bool m_is_mockable_chain;
    CCheckpointData checkpointData;
    MapAssumeutxo m_assumeutxo_data;
    ChainTxData chainTxData;
};

#endif // BITCOIN_KERNEL_CHAINPARAMS_H
