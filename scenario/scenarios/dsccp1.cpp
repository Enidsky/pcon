#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ndnSIM-module.h"
#include "../extensions/PCONStrategy.hpp"

namespace ns3
{

    int
    main(int argc, char *argv[])
    {
        CommandLine cmd;
        cmd.Parse(argc, argv);

        AnnotatedTopologyReader topologyReader("", 25);
        topologyReader.SetFileName("topologies/dsccp1.txt");
        topologyReader.Read();

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.setPolicy("nfd::cs::lru");
        // ndnHelper.setCsSize(10000);
        ndnHelper.setCsSize(0);
        ndnHelper.InstallAll();

        // Choosing forwarding strategy
        ndn::StrategyChoiceHelper::InstallAll<nfd::fw::PCONStrategy>("/");
        // ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
        // ndn::StrategyChoiceHelper::Install(Names::Find<Node>("R1"), "/", "/localhost/nfd/strategy/random/%FD%01");
        // ndn::StrategyChoiceHelper::InstallAll<nfd::fw::DSCCPStrategy>("/");

        // Installing global routing interface on all nodes
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.InstallAll();

        // Getting containers for the consumer/producer
        Ptr<Node> C1 = Names::Find<Node>("C1");
        Ptr<Node> C2 = Names::Find<Node>("C2");
        Ptr<Node> C3 = Names::Find<Node>("C3");
        Ptr<Node> C4 = Names::Find<Node>("C4");

        Ptr<Node> P1 = Names::Find<Node>("P1");
        Ptr<Node> P2 = Names::Find<Node>("P2");
        Ptr<Node> P3 = Names::Find<Node>("P3");
        Ptr<Node> P4 = Names::Find<Node>("P4");

        // ndn::AppHelper consumerHelper("ns3::ndn::ConsumerDSCCP");
        // consumerHelper.SetAttribute("Dsz", UintegerValue(8696));

        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
        // consumerHelper.SetAttribute("Dsz", UintegerValue(8696));
        // consumerHelper.SetAttribute("NumberOfContents", UintegerValue(10000));

        // C1 => EF
        // consumerHelper.SetAttribute("Tos", UintegerValue(2));
        for (int i = 0; i < 10; i++)
        {
            consumerHelper.SetPrefix("/A" + std::to_string(i));
            consumerHelper.Install(C1);
        }

        // C2 => AF2
        // consumerHelper.SetAttribute("Tos", UintegerValue(5));
        for (int i = 0; i < 10; i++)
        {
            consumerHelper.SetPrefix("/B" + std::to_string(i));
            consumerHelper.Install(C2);
        }

        // C3 => AF1
        // consumerHelper.SetAttribute("Tos", UintegerValue(6));
        for (int i = 0; i < 10; i++)
        {
            consumerHelper.SetPrefix("/C" + std::to_string(i));
            consumerHelper.Install(C3);
        }

        // C4 => BE
        // consumerHelper.SetAttribute("Tos", UintegerValue(7));
        for (int i = 0; i < 10; i++)
        {
            consumerHelper.SetPrefix("/D" + std::to_string(i));
            consumerHelper.Install(C4);
        }

        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

        // Register /dst1 prefix with global routing controller and
        // install producer that will satisfy Interests in /dst1 namespace
        // P1

        // P1 => EF => /A*
        for (int i = 0; i < 10; i++) {
            producerHelper.SetPrefix("/A" + std::to_string(i));
            ndnGlobalRoutingHelper.AddOrigins("/A" + std::to_string(i), P1);
            producerHelper.Install(P1);
        }

        // P2 => AF2 => /B*
        for (int i = 0; i < 10; i++) {
            producerHelper.SetPrefix("/B" + std::to_string(i));
            ndnGlobalRoutingHelper.AddOrigins("/B" + std::to_string(i), P2);
            producerHelper.Install(P2);
        }

        // P3 => AF1 => /C*
        for (int i = 0; i < 10; i++) {
            producerHelper.SetPrefix("/C" + std::to_string(i));
            ndnGlobalRoutingHelper.AddOrigins("/C" + std::to_string(i), P3);
            producerHelper.Install(P3);
        }

        // P4 => BE => /D*
        for (int i = 0; i < 10; i++) {
            producerHelper.SetPrefix("/D" + std::to_string(i));
            ndnGlobalRoutingHelper.AddOrigins("/D" + std::to_string(i), P4);
            producerHelper.Install(P4);
        }


        // Calculate all possible routes and install FIBs
        ndn::GlobalRoutingHelper::CalculateRoutes();

        Simulator::Stop(Seconds(20.0));

        ndn::L3RateTracer::InstallAll("dsccp1_throughput.txt", Seconds(0.5));
        ndn::AppDelayTracer::InstallAll("dsccp1_delay.txt");
        L2RateTracer::InstallAll("dsccp1_drop.txt", Seconds(0.5));
        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int main(int argc, char *argv[])
{
    return ns3::main(argc, argv);
}
