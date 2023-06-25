# assumeutxo

Assumeutxo is a feature that allows fast bootstrapping of a validating bitcoind
instance with a very similar security model to assumevalid.

The RPC commands `dumptxoutset` and `loadtxoutset` (yet to be merged) are used to
respectively generate and load UTXO snapshots. The utility script
`./contrib/devtools/utxo_snapshot.sh` may be of use.

"assumeutxo"是一个比特币的术语，它指的是一种特定类型的交易。在比特币的交易系统中，每个区块都包含了一定数量的交易记录，这些交易记录被称为"未确认交易"（unconfirmed transactions）。
当一个人想要在比特币网络上进行一笔交易时，他们需要向网络发送一个交易请求，并支付一定的交易费用。这个交易请求被称为"猜测交易"（guess transaction），因为在发送交易请求时，交易者并不知道其他节点是否会接受这个请求。
如果其他节点接受了这个请求，那么这个交易就会被确认为"已确认交易"（confirmed transactions）。这时，这个交易就可以被记录在区块中，并且所有人都可以查看这个交易的历史记录。
而"assumeutxo"是指在没有确认交易的情况下进行的交易。在这种情况下，交易者发送了一个猜测交易，但是没有得到其他节点的确认，因此这笔交易并没有被记录在区块中。这意味着这笔交易没有被其他人所认可，交易的安全性和有效性无法得到保障。
因此，在进行比特币交易时，需要确保所有的交易都已经被确认，并且不存在"assumeutxo"的情况

## General background

- [assumeutxo proposal](https://github.com/jamesob/assumeutxo-docs/tree/2019-04-proposal/proposal)
- [Github issue](https://github.com/bitcoin/bitcoin/issues/15605)
- [draft PR](https://github.com/bitcoin/bitcoin/pull/15606)

## Design notes

- A new block index `nStatus` flag is introduced, `BLOCK_ASSUMED_VALID`, to mark block
  index entries that are required to be assumed-valid by a chainstate created
  from a UTXO snapshot. This flag is mostly used as a way to modify certain
  CheckBlockIndex() logic to account for index entries that are pending validation by a
  chainstate running asynchronously in the background. We also use this flag to control
  which index entries are added to setBlockIndexCandidates during LoadBlockIndex().

- The concept of UTXO snapshots is treated as an implementation detail that lives
  behind the ChainstateManager interface. The external presentation of the changes
  required to facilitate the use of UTXO snapshots is the understanding that there are
  now certain regions of the chain that can be temporarily assumed to be valid (using
  the nStatus flag mentioned above). In certain cases, e.g. wallet rescanning, this is
  very similar to dealing with a pruned chain.

  Logic outside ChainstateManager should try not to know about snapshots, instead
  preferring to work in terms of more general states like assumed-valid.


## Chainstate phases

Chainstate within the system goes through a number of phases when UTXO snapshots are
used, as managed by `ChainstateManager`. At various points there can be multiple
`Chainstate` objects in existence to facilitate both maintaining the network tip and
performing historical validation of the assumed-valid chain.

It is worth noting that though there are multiple separate chainstates, those
chainstates share use of a common block index (i.e. they hold the same `BlockManager`
reference).

The subheadings below outline the phases and the corresponding changes to chainstate
data.

### "Normal" operation via initial block download

`ChainstateManager` manages a single Chainstate object, for which
`m_snapshot_blockhash` is null. This chainstate is (maybe obviously)
considered active. This is the "traditional" mode of operation for bitcoind.

|    |    |
| ---------- | ----------- |
| number of chainstates | 1 |
| active chainstate | ibd |

### User loads a UTXO snapshot via `loadtxoutset` RPC

UTXO snapshot是指对比特币系统中所有未确认的UTXO进行快照的过程。在这个过程中，系统会对所有未确认的UTXO进行检索和统计，并将其存储在一个特定的数据结构中。这个数据结构被称为UTXO set，其中包括了所有未确认的UTXO的状态、输入值、输出值和交易费用等信息。
因此，UTXO snapshot是比特币系统中非常重要的一个技术，它用于记录每个区块中未确认的交易，并为所有交易的历史记录提供了基础

`ChainstateManager` initializes a new chainstate (see `ActivateSnapshot()`) to load the
snapshot contents into. During snapshot load and validation (see
`PopulateAndValidateSnapshot()`), the new chainstate is not considered active and the
original chainstate remains in use as active.

|    |    |
| ---------- | ----------- |
| number of chainstates | 2 |
| active chainstate | ibd |

Once the snapshot chainstate is loaded and validated, it is promoted to active
chainstate and a sync to tip begins. A new chainstate directory is created in the
datadir for the snapshot chainstate called `chainstate_snapshot`.

When this directory is present in the datadir, the snapshot chainstate will be detected
and loaded as active on node startup (via `DetectSnapshotChainstate()`).

A special file is created within that directory, `base_blockhash`, which contains the
serialized `uint256` of the base block of the snapshot. This is used to reinitialize
the snapshot chainstate on subsequent inits. Otherwise, the directory is a normal
leveldb database.

|    |    |
| ---------- | ----------- |
| number of chainstates | 2 |
| active chainstate | snapshot |

The snapshot begins to sync to tip from its base block, technically in parallel with
the original chainstate, but it is given priority during block download and is
allocated most of the cache (see `MaybeRebalanceCaches()` and usages) as our chief
goal is getting to network tip.

**Failure consideration:** if shutdown happens at any point during this phase, both
chainstates will be detected during the next init and the process will resume.

### Snapshot chainstate hits network tip

Once the snapshot chainstate leaves IBD, caches are rebalanced
(via `MaybeRebalanceCaches()` in `ActivateBestChain()`) and more cache is given
to the background chainstate, which is responsible for doing full validation of the
assumed-valid parts of the chain.

**Note:** at this point, ValidationInterface callbacks will be coming in from both
chainstates. Considerations here must be made for indexing, which may no longer be happening
sequentially.

### Background chainstate hits snapshot base block

Once the tip of the background chainstate hits the base block of the snapshot
chainstate, we stop use of the background chainstate by setting `m_disabled`, in
`CompleteSnapshotValidation()`, which is checked in `ActivateBestChain()`). We hash the
background chainstate's UTXO set contents and ensure it matches the compiled value in
`CMainParams::m_assumeutxo_data`.

|    |    |
| ---------- | ----------- |
| number of chainstates | 2 (ibd has `m_disabled=true`) |
| active chainstate | snapshot |

The background chainstate data lingers on disk until the program is restarted.

### Bitcoind restarts sometime after snapshot validation has completed

After a shutdown and subsequent restart, `LoadChainstate()` cleans up the background
chainstate with `ValidatedSnapshotCleanup()`, which renames the `chainstate_snapshot`
datadir as `chainstate` and removes the now unnecessary background chainstate data.

|    |    |
| ---------- | ----------- |
| number of chainstates | 1 |
| active chainstate | ibd (was snapshot, but is now fully validated) |

What began as the snapshot chainstate is now indistinguishable from a chainstate that
has been built from the traditional IBD process, and will be initialized as such.

A file will be left in `chainstate/base_blockhash`, which indicates that the
chainstate, even though now fully validated, was originally started from a snapshot
with the corresponding base blockhash.
