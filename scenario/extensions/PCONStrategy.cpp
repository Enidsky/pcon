#include "PCONStrategy.hpp"
#include <ndn-cxx/lp/tags.hpp>

NFD_LOG_INIT(PCONStrategy);
namespace nfd
{
    namespace fw
    {
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //// PCONStrategy
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        const time::milliseconds PCONStrategy::RETX_SUPPRESSION_INITIAL(10);
        const time::milliseconds PCONStrategy::RETX_SUPPRESSION_MAX(250);

        PCONStrategy::PCONStrategy(nfd::Forwarder &forwarder, const ndn::Name &name)
            : Strategy(forwarder), ProcessNackTraits<PCONStrategy>(this), m_retxSuppression(RETX_SUPPRESSION_INITIAL,
                                                                                            RetxSuppressionExponential::DEFAULT_MULTIPLIER,
                                                                                            RETX_SUPPRESSION_MAX)
        {
            this->setInstanceName(makeInstanceName(name, getStrategyName()));
        }

        const Name &
        PCONStrategy::getStrategyName()
        {
            static Name strategyName("/localhost/nfd/strategy/PCON/%FD%01");
            return strategyName;
        }

        void
        PCONStrategy::afterReceiveInterest(const nfd::FaceEndpoint &ingress, const ndn::Interest &interest,
                                           const std::shared_ptr<nfd::pit::Entry> &pitEntry)
        {
            const fib::Entry &fibEntry = this->lookupFib(*pitEntry);
            const fib::NextHopList &nexthops = fibEntry.getNextHops();
            MtForwardingInfo *measurementInfo = this->getPrefixMeasurements(fibEntry);

            if (measurementInfo == nullptr)
            {
                measurementInfo = this->addPrefixMeasurements(fibEntry);
                const Name prefixName = fibEntry.getPrefix();
                measurementInfo->setPrefix(prefixName.toUri());
                initializeForwMap(measurementInfo, nexthops);
            }

            RetxSuppressionResult suppression = m_retxSuppression.decidePerPitEntry(*pitEntry);
            if (suppression == RetxSuppressionResult::SUPPRESS)
            {
                NFD_LOG_DEBUG(interest << " from=" << ingress << " suppressed");
                return;
            }

            Face *outFace = nullptr;
            if (ingress.face.getScope() != ndn::nfd::FACE_SCOPE_NON_LOCAL)
            {
                auto selected = std::find_if(nexthops.begin(), nexthops.end(), [&](const auto &nexthop) {
                    return isNextHopEligible(ingress.face, interest, nexthop, pitEntry);
                });
                if (selected != nexthops.end())
                {
                    outFace = &((*selected).getFace());
                }
            }
            else
            {
                // Random number between 0 and 1.
                double r = ((double)rand() / (RAND_MAX));

                double percSum = 0;

                // Add all eligbile faces to list (excludes current downstream)
                std::vector<Face *> eligbleFaces;
                for (auto &n : fibEntry.getNextHops())
                {
                    if (isNextHopEligible(ingress.face, interest, n, pitEntry))
                    {
                        // Add up percentage Sum.
                        percSum += measurementInfo->getforwPerc(n.getFace().getId());
                        eligbleFaces.push_back(&n.getFace());
                    }
                }

                if (eligbleFaces.size() < 1)
                {
                    return;
                }
                else if (eligbleFaces.size() == 1)
                {
                    outFace = eligbleFaces.front();
                }
                else
                {
                    // Choose face according to current forwarding percentage:
                    double forwPerc = 0;
                    for (auto face : eligbleFaces)
                    {
                        forwPerc += measurementInfo->getforwPerc(face->getId()) / percSum;
                        if (r < forwPerc)
                        {
                            outFace = face;
                            break;
                        }
                    }
                }
            }

            if (outFace == nullptr)
            {
                lp::NackHeader nackHeader;
                nackHeader.setReason(lp::NackReason::NO_ROUTE);
                this->sendNack(pitEntry, ingress, nackHeader);
                this->rejectPendingInterest(pitEntry);
                return;
            }
            else
            {
                this->sendInterest(pitEntry, FaceEndpoint(*outFace, 0), interest);
            }

        }

        void
        PCONStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry> &pitEntry,
                                            const FaceEndpoint &ingress, const Data &data)
        {
            Name currentPrefix;
            MtForwardingInfo *measurementInfo;
            std::tie(currentPrefix, measurementInfo) = this->findPrefixMeasurementsLPM(
                *pitEntry);

            if (data.getCongestionMark() > 0 && ingress.face.getId() > 256)
            {
                // std::cout << ">>2: " << data.getCongestionMark() << std::endl;
                double fwPerc = measurementInfo->getforwPerc(ingress.face.getId());
                double change_perc = CHANGE_PER_MARK * fwPerc;
                this->reduceFwPerc(measurementInfo, ingress.face.getId(), change_perc);
            }
        }

        void PCONStrategy::afterReceiveNack(const nfd::FaceEndpoint &ingress, const ndn::lp::Nack &nack,
                                            const std::shared_ptr<nfd::pit::Entry> &pitEntry)
        {
            this->processNack(ingress.face, nack, pitEntry);
        }

        void
        PCONStrategy::initializeForwMap(MtForwardingInfo *measurementInfo,
                                        const fib::NextHopList &nextHops)
        {
            int lowestId = std::numeric_limits<int>::max();
            int minCost = std::numeric_limits<int>::max();

            for (auto &n : nextHops)
            {
                if (n.getCost() < minCost)
                {
                    minCost = n.getCost();
                    lowestId = n.getFace().getId();
                }
            }

            for (auto &n : nextHops)
            {
                double perc = 0.0;
                if (n.getFace().getId() == lowestId)
                {
                    perc = 1.0;
                }
                measurementInfo->setforwPerc(n.getFace().getId(), perc);
            }
        }
    }
}