#ifndef NFD_DAEMON_FW_PCON_STRATEGY_HPP
#define NFD_DAEMON_FW_PCON_STRATEGY_HPP

#include <boost/random/mersenne_twister.hpp>
#include "face/face.hpp"
#include "fw/strategy.hpp"
#include "fw/retx-suppression-exponential.hpp"
#include "fw/algorithm.hpp"
#include "fw/process-nack-traits.hpp"
#include <unordered_map>
#include <limits>
#include "mt-forward-info.hpp"

namespace nfd
{
    namespace fw
    {
        class PCONStrategy : public Strategy, public ProcessNackTraits<PCONStrategy>
        {
        public:
            explicit PCONStrategy(Forwarder &forwarder, const Name &name = getStrategyName());

            ~PCONStrategy() override = default;

            void
            afterReceiveInterest(const FaceEndpoint &ingress, const Interest &interest,
                                 const shared_ptr<pit::Entry> &pitEntry) override;

            void
            beforeSatisfyInterest(const shared_ptr<pit::Entry> &pitEntry,
                                  const FaceEndpoint &ingress, const Data &data) override;

            void
            afterReceiveNack(const FaceEndpoint &ingress, const lp::Nack &nack,
                             const shared_ptr<pit::Entry> &pitEntry) override;

            static const Name &
            getStrategyName();

        protected:
            friend ProcessNackTraits<PCONStrategy>;

        private:
            void
            initializeForwMap(MtForwardingInfo *measurementInfo,
                              const fib::NextHopList &nextHops);

            MtForwardingInfo *
            getPrefixMeasurements(const fib::Entry &fibEntry)
            {
                std::string key = fibEntry.getPrefix().getPrefix(1).toUri();
                if (this->innerMap.count(key) == 0)
                {
                    return nullptr;
                }
                else
                {
                    return this->innerMap[key].get();
                }
            }

            MtForwardingInfo *
            addPrefixMeasurements(const fib::Entry &fibEntry)
            {
                std::string key = fibEntry.getPrefix().getPrefix(1).toUri();
                if (this->innerMap.count(key) == 0)
                {
                    this->innerMap[key] = make_unique<MtForwardingInfo>();
                }
                return this->innerMap[key].get();
                // measurements::Entry* me = measurements.get(fibEntry);
                // // std::cout << fibEntry.getPrefix().toUri() << std::endl;
                // measurements.extendLifetime(*me, 8_s);
                // return me->insertStrategyInfo<MtForwardingInfo>().first;
            }

            std::tuple<Name, MtForwardingInfo *>
            findPrefixMeasurementsLPM(const pit::Entry &pitEntry)
            {

                std::string key = pitEntry.getName().getPrefix(1).toUri();
                if (this->innerMap.count(key) == 0) {
                    return std::forward_as_tuple(Name(), nullptr);
                }

                return std::forward_as_tuple(pitEntry.getName().getPrefix(1), this->innerMap[key].get());
                // measurements::Entry *me = measurements.findLongestPrefixMatch(pitEntry);
                // if (me == nullptr)
                // {
                //     std::cout << "Name " << pitEntry.getName().toUri() << " not found!\n";
                //     return std::forward_as_tuple(Name(), nullptr);
                // }
                // return std::forward_as_tuple(me->getName(), me->getStrategyInfo<MtForwardingInfo>());
            }

            static void
            reduceFwPerc(MtForwardingInfo *forwInfo,
                         const FaceId reducedFaceId,
                         const double change)
            {
                if (forwInfo->getFaceCount() == 1)
                {
                    std::cout << "reduce1" << std::endl;
                    return;
                }
                // Reduction is at most the current forwarding percentage of the face that is reduced.
                double changeRate = 0 - std::min(change, forwInfo->getforwPerc(reducedFaceId));

                std::cout << "reduce2: " << changeRate << std::endl;
                // Decrease fw percentage of the given face:
                forwInfo->increaseforwPerc(reducedFaceId, changeRate);
                double sumFWPerc = 0;
                sumFWPerc += forwInfo->getforwPerc(reducedFaceId);
                const auto forwMap = forwInfo->getForwPercMap();

                //		std::cout << "\n";
                for (auto f : forwMap)
                {
                    auto tempChangeRate = changeRate;
                    auto &faceId = f.first;
                    if (faceId == reducedFaceId)
                    { // Do nothing. Percentage has already been added.
                    }
                    else
                    {
                        // Increase forwarding percentage of all other faces by and equal amount.
                        tempChangeRate = std::abs(changeRate / (double)(forwMap.size() - 1));
                        forwInfo->increaseforwPerc((faceId), tempChangeRate);
                        sumFWPerc += forwInfo->getforwPerc(faceId);
                    }
                }

                // if (sumFWPerc < 0.999 || sumFWPerc > 1.001)
                // {
                //     std::cout << StrHelper::getTime() << "ERROR! Sum of fw perc out of range: " << sumFWPerc
                //               << "\n";
                // }
            }

        private:
            static const time::milliseconds RETX_SUPPRESSION_INITIAL;
            static const time::milliseconds RETX_SUPPRESSION_MAX;
            double CHANGE_PER_MARK = 0.1;
            std::unordered_map<std::string, unique_ptr<MtForwardingInfo>> innerMap;
            RetxSuppressionExponential m_retxSuppression;
        };
    }
}

#endif