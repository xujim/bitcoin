// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_PSBT_H
#define BITCOIN_NODE_PSBT_H

#include <psbt.h>

#include <optional>

namespace node {
/**
 * Holds an analysis of one input from a PSBT
 * 关于什么是PSBT：http://www.theblockbeats.info/news/35405
 * PSBT 是一个比特币标准，用来协助未签名交易（unsigned transactions）的传输；该标准可以让多方更容易签名同一笔交易。
 * PSBT 给比特币社区提供了许多好处，也让此前的复杂协议得到简化，变得更容易验证。
互通性。PSBT 的设计初衷是强化钱包和其它比特币软件的互通性，让交易可以更容易地在钱包和节点间传输。PSBT 在很大程度上已经成功了，它获得了所有主要的钱包供应商和节点软件的支持，也就是已经得到了行业的接受。
离线签名。PSBT 格式提供了有用的元数据，可以协助冷存储设备验证即将被签名的交易相关的地址和金额。这使得从冷存储设备发起签名变得更加安全，而且 观察钱包构造交易-冷钱包签名-比特币节点广播交易 的整个过程也变得更加容易。
多签名流程。因为 PSBT 让一个部分签名的比特币交易变得更容易传输和理解，多方（或者说多个设备）签名一笔交易也变得更容易、更安全，因此多签名技术也变得更容易使用。用户友好型多签名钱包将给比特币社区带来进一步的好处，
包括更好的隐私性、安全性和私钥丢失抗性。
多方交易。PSBT 对想要签名同一笔交易的协作多方尤为实用。比如，CoinJoin、CoinSwap 和 PayJoin 协议，都要求多方签名同一笔交易。PSBT 格式提供了构造交易、在多个签名者之间传输交易、组装成最终交易的方法
 */
struct PSBTInputAnalysis {
    bool has_utxo; //!< Whether we have UTXO information for this input
    bool is_final; //!< Whether the input has all required information including signatures
    PSBTRole next; //!< Which of the BIP 174 roles needs to handle this input next

    std::vector<CKeyID> missing_pubkeys; //!< Pubkeys whose BIP32 derivation path is missing
    std::vector<CKeyID> missing_sigs;    //!< Pubkeys whose signatures are missing
    uint160 missing_redeem_script;       //!< Hash160 of redeem script, if missing
    uint256 missing_witness_script;      //!< SHA256 of witness script, if missing
};

/**
 * Holds the results of AnalyzePSBT (miscellaneous information about a PSBT)
 */
struct PSBTAnalysis {
    std::optional<size_t> estimated_vsize;      //!< Estimated weight of the transaction
    std::optional<CFeeRate> estimated_feerate;  //!< Estimated feerate (fee / weight) of the transaction
    std::optional<CAmount> fee;                 //!< Amount of fee being paid by the transaction
    std::vector<PSBTInputAnalysis> inputs;      //!< More information about the individual inputs of the transaction
    PSBTRole next;                              //!< Which of the BIP 174 roles needs to handle the transaction next
    std::string error;                          //!< Error message

    void SetInvalid(std::string err_msg)
    {
        estimated_vsize = std::nullopt;
        estimated_feerate = std::nullopt;
        fee = std::nullopt;
        inputs.clear();
        next = PSBTRole::CREATOR;
        error = err_msg;
    }
};

/**
 * Provides helpful miscellaneous information about where a PSBT is in the signing workflow.
 *
 * @param[in] psbtx the PSBT to analyze
 * @return A PSBTAnalysis with information about the provided PSBT.
 */
PSBTAnalysis AnalyzePSBT(PartiallySignedTransaction psbtx);
} // namespace node

#endif // BITCOIN_NODE_PSBT_H
