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
        topologyReader.SetFileName("topologies/pcon-2.txt");
        topologyReader.Read();

        // Install NDN stack on all nodes
        ndn::StackHelper ndnHelper;
        ndnHelper.setPolicy("nfd::cs::lru");
        // ndnHelper.setCsSize(10000);
        ndnHelper.setCsSize(0);
        ndnHelper.InstallAll();

        // Choosing forwarding strategy
        ndn::StrategyChoiceHelper::InstallAll<nfd::fw::PCONStrategy>("/");

        // Installing global routing interface on all nodes
        ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
        ndnGlobalRoutingHelper.InstallAll();

        Ptr<Node> P1 = Names::Find<Node>("P1");
        Ptr<Node> P2 = Names::Find<Node>("P2");
        Ptr<Node> P3 = Names::Find<Node>("P3");

        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
        consumerHelper.SetAttribute("Dsz", UintegerValue(8696));
        consumerHelper.SetAttribute("CcAlgorithm", StringValue("BIC"));


        // C1
        consumerHelper.SetPrefix("/A");
        consumerHelper.Install("C1");

        // C2
        consumerHelper.SetPrefix("/B");
        consumerHelper.SetAttribute("DelayStart", UintegerValue(100000));
        consumerHelper.Install("C2");

        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

        producerHelper.SetPrefix("/A");
        producerHelper.Install(P1);
        producerHelper.Install(P2);
        producerHelper.Install(P3);


        producerHelper.SetPrefix("/B");
        producerHelper.Install(P1);
        producerHelper.Install(P2);
        producerHelper.Install(P3);

        // Calculate all possible routes and install FIBs
        // ndn::GlobalRoutingHelper::CalculateRoutes();

        ndn::FibHelper::AddRoute("C1", "/A", "R1", 0);
        ndn::FibHelper::AddRoute("C2", "/B", "R1", 0);

        ndn::FibHelper::AddRoute("R1", "/A", "R2", 0);
        ndn::FibHelper::AddRoute("R1", "/B", "R2", 0);

        ndn::FibHelper::AddRoute("R1", "/A", "R3", 0);
        ndn::FibHelper::AddRoute("R1", "/B", "R3", 0);

        ndn::FibHelper::AddRoute("R1", "/A", "R4", 0);
        ndn::FibHelper::AddRoute("R1", "/B", "R4", 0);

        ndn::FibHelper::AddRoute("R2", "/A", "P1", 0);
        ndn::FibHelper::AddRoute("R2", "/B", "P1", 0);

        ndn::FibHelper::AddRoute("R4", "/A", "P2", 0);
        ndn::FibHelper::AddRoute("R4", "/B", "P2", 0);

        ndn::FibHelper::AddRoute("R3", "/A", "P3", 0);
        ndn::FibHelper::AddRoute("R3", "/B", "P3", 0);


        Simulator::Stop(Seconds(200));

        ndn::L3RateTracer::InstallAll("pcon-test-2-throughput.txt", Seconds(1));
        ndn::AppDelayTracer::InstallAll("pcon-test-2-delay.txt");
        L2RateTracer::InstallAll("pcon-test-2-drop.txt", Seconds(1));
        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int main(int argc, char *argv[])
{
    return ns3::main(argc, argv);
}
