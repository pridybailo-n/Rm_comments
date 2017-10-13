




#ifndef BITCOIN_MINER_H
#define BITCOIN_MINER_H

#include "primitives/block.h"
#include "txmempool.h"

#include <stdint.h>
#include <memory>
#include "boost/multi_index_container.hpp"
#include "boost/multi_index/ordered_index.hpp"

class CBlockIndex;
class CChainParams;
class CScript;

namespace Consensus { struct Params; };

static const bool DEFAULT_PRINTPRIORITY = false;

struct CBlockTemplate
{
    CBlock block;
    std::vector<CAmount> vTxFees;
    std::vector<int64_t> vTxSigOpsCost;
    std::vector<unsigned char> vchCoinbaseCommitment;
};



struct CTxMemPoolModifiedEntry {
    explicit CTxMemPoolModifiedEntry(CTxMemPool::txiter entry)
    {
        iter = entry;
        nSizeWithAncestors = entry->GetSizeWithAncestors();
        nModFeesWithAncestors = entry->GetModFeesWithAncestors();
        nSigOpCostWithAncestors = entry->GetSigOpCostWithAncestors();
    }

    CTxMemPool::txiter iter;
    uint64_t nSizeWithAncestors;
    CAmount nModFeesWithAncestors;
    int64_t nSigOpCostWithAncestors;
};


struct CompareCTxMemPoolIter {
    bool operator()(const CTxMemPool::txiter& a, const CTxMemPool::txiter& b) const
    {
        return &(*a) < &(*b);
    }
};

struct modifiedentry_iter {
    typedef CTxMemPool::txiter result_type;
    result_type operator() (const CTxMemPoolModifiedEntry &entry) const
    {
        return entry.iter;
    }
};




struct CompareModifiedEntry {
    bool operator()(const CTxMemPoolModifiedEntry &a, const CTxMemPoolModifiedEntry &b)
    {
        double f1 = (double)a.nModFeesWithAncestors * b.nSizeWithAncestors;
        double f2 = (double)b.nModFeesWithAncestors * a.nSizeWithAncestors;
        if (f1 == f2) {
            return CTxMemPool::CompareIteratorByHash()(a.iter, b.iter);
        }
        return f1 > f2;
    }
};




struct CompareTxIterByAncestorCount {
    bool operator()(const CTxMemPool::txiter &a, const CTxMemPool::txiter &b)
    {
        if (a->GetCountWithAncestors() != b->GetCountWithAncestors())
            return a->GetCountWithAncestors() < b->GetCountWithAncestors();
        return CTxMemPool::CompareIteratorByHash()(a, b);
    }
};

typedef boost::multi_index_container<
    CTxMemPoolModifiedEntry,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<
            modifiedentry_iter,
            CompareCTxMemPoolIter
        >,
        
        boost::multi_index::ordered_non_unique<
            
            boost::multi_index::tag<ancestor_score>,
            boost::multi_index::identity<CTxMemPoolModifiedEntry>,
            CompareModifiedEntry
        >
    >
> indexed_modified_transaction_set;

typedef indexed_modified_transaction_set::nth_index<0>::type::iterator modtxiter;
typedef indexed_modified_transaction_set::index<ancestor_score>::type::iterator modtxscoreiter;

struct update_for_parent_inclusion
{
    explicit update_for_parent_inclusion(CTxMemPool::txiter it) : iter(it) {}

    void operator() (CTxMemPoolModifiedEntry &e)
    {
        e.nModFeesWithAncestors -= iter->GetFee();
        e.nSizeWithAncestors -= iter->GetTxSize();
        e.nSigOpCostWithAncestors -= iter->GetSigOpCost();
    }

    CTxMemPool::txiter iter;
};


class BlockAssembler
{
private:
    
    std::unique_ptr<CBlockTemplate> pblocktemplate;
    
    CBlock* pblock;

    
    bool fIncludeWitness;
    unsigned int nBlockMaxWeight, nBlockMaxSize;
    bool fNeedSizeAccounting;
    CFeeRate blockMinFeeRate;

    
    uint64_t nBlockWeight;
    uint64_t nBlockSize;
    uint64_t nBlockTx;
    uint64_t nBlockSigOpsCost;
    CAmount nFees;
    CTxMemPool::setEntries inBlock;

    
    int nHeight;
    int64_t nLockTimeCutoff;
    const CChainParams& chainparams;

public:
    struct Options {
        Options();
        size_t nBlockMaxWeight;
        size_t nBlockMaxSize;
        CFeeRate blockMinFeeRate;
    };

    explicit BlockAssembler(const CChainParams& params);
    BlockAssembler(const CChainParams& params, const Options& options);

    
    std::unique_ptr<CBlockTemplate> CreateNewBlock(const CScript& scriptPubKeyIn, bool fMineWitnessTx=true);

private:
    
    
    void resetBlock();
    
    void AddToBlock(CTxMemPool::txiter iter);

    
    
    void addPackageTxs(int &nPackagesSelected, int &nDescendantsUpdated);

    
    
    void onlyUnconfirmed(CTxMemPool::setEntries& testSet);
    
    bool TestPackage(uint64_t packageSize, int64_t packageSigOpsCost) const;
    
    bool TestPackageTransactions(const CTxMemPool::setEntries& package);
    
    bool SkipMapTxEntry(CTxMemPool::txiter it, indexed_modified_transaction_set &mapModifiedTx, CTxMemPool::setEntries &failedTx);
    
    void SortForBlock(const CTxMemPool::setEntries& package, CTxMemPool::txiter entry, std::vector<CTxMemPool::txiter>& sortedEntries);
    
    int UpdatePackagesForAdded(const CTxMemPool::setEntries& alreadyAdded, indexed_modified_transaction_set &mapModifiedTx);
};


void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

#endif 
