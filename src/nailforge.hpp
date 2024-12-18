#ifndef NAIL_FORGE_HPP
#define NAIL_FORGE_HPP
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include <memory>
#include <omp.h>

#include "Alphabet/Alphabet.hpp"
extern "C" {
#include "AwFmIndex.h"
#include "FastaVector.h"
#include "FastaVectorMetadataVector.h"
#include "FastaVectorString.h"
#include "p7HmmReader.h"
}

namespace NailForge {

    enum class SearchType {
        Standard,
        ComplementStrand,
        DualStrand
    };

    struct SearchParams {
        float mainDiagonalThresholdScore;
        float extensionPValue;
        float maxSeqHitsPerMillion;
        uint32_t maximumHitLength;
        uint32_t flankExtensionLength;
        static SearchParams defaultParams(const NailForge::Alphabet alphabet) {
            SearchParams searchParams;
            if (alphabet == NailForge::Alphabet::Amino) {
                searchParams.mainDiagonalThresholdScore = 10;
                searchParams.extensionPValue = 0.001;
                searchParams.maximumHitLength = 5;
                searchParams.maxSeqHitsPerMillion = 100;
                searchParams.flankExtensionLength = 8;
            }
            else {
                searchParams.mainDiagonalThresholdScore = 14;
                searchParams.extensionPValue = 0.001;
                searchParams.maximumHitLength = 15;
                searchParams.maxSeqHitsPerMillion = 100;
                searchParams.flankExtensionLength = 16;
            }
            return searchParams;
        }
    };

    enum class ReturnCode {
        Success = 0,
        GeneralFailure = -1,
        FileNotFound = -2,
        FileReadError = -3,
        FileWriteError = -4,
        FmIndexError = -5,
        MemoryError = -6,
        FileFormatError = -7,
        FileAlreadyExists = -8,
        AlphabetMismatch = -9,
        NotImplemented = -10,
        AllocationFailure = -11
    };

    struct ModelSeqKey {
        uint32_t modelIdx;
        uint32_t sequenceIdx;
        bool operator <(const ModelSeqKey& otherKey) {
            if (modelIdx == otherKey.modelIdx) {
                return sequenceIdx < otherKey.sequenceIdx;
            }
            else {
                return modelIdx < otherKey.modelIdx;
            }
        }
    };
    struct AlignmentSeed {
        uint64_t sequencePosition;
        uint32_t modelPosition;
        uint32_t sequenceIdx;
        float fullScore;
        AlignmentSeed(const uint64_t sequencePosition, const uint32_t modelPosition,
            const uint32_t sequenceIdx, const float fullScore) :
            sequencePosition(sequencePosition), modelPosition(modelPosition),
            sequenceIdx(sequenceIdx), fullScore(fullScore) {}
        bool operator <(const AlignmentSeed& otherSeed) {
            if (sequenceIdx != otherSeed.sequenceIdx) {
                return sequenceIdx < otherSeed.sequenceIdx;
            }
            else {
                uint64_t thisAntiDiagonal = modelPosition + sequencePosition;
                uint64_t otherAntiDiagonal = otherSeed.modelPosition + otherSeed.sequencePosition;
                if (thisAntiDiagonal != otherAntiDiagonal) {
                    return thisAntiDiagonal < otherAntiDiagonal;
                }
                else {
                    int64_t thisDiagonal = (int64_t)modelPosition - (int64_t)sequencePosition;
                    int64_t otherDiagonal = (int64_t)otherSeed.modelPosition - (int64_t)otherSeed.sequencePosition;
                    return thisDiagonal < otherDiagonal;
                }
            }
        }
    };

    // returns a text description of the return code.
    std::string_view returnCodeDescription(const NailForge::ReturnCode rc);

    NailForge::ReturnCode createFmIndex(const char* fastaFileSrc, const char* fmIndexFileSrc,
        const NailForge::Alphabet& alphabet, const uint8_t suffixArrayCompressionRatio);

    NailForge::ReturnCode filterWithHmmFile(const char* hmmFileSrc, const char* fastaFileSrc, const char* fmIndexFileSrc,
        const SearchParams& params, const SearchType searchType, const uint8_t numThreads,
        std::vector<std::vector<AlignmentSeed>>& primarySeedList, std::vector<std::vector<AlignmentSeed>>& complementSeedList,
        float &searchTimedDuration);
}

#endif