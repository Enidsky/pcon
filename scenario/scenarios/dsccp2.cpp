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
        topologyReader.SetFileName("topologies/dsccp2.txt");
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

        // Getting containers for the consumer/producer
        Ptr<Node> C1 = Names::Find<Node>("C1");
        Ptr<Node> C2 = Names::Find<Node>("C2");
        Ptr<Node> C3 = Names::Find<Node>("C3");

        Ptr<Node> P1 = Names::Find<Node>("P1");
        Ptr<Node> P2 = Names::Find<Node>("P2");
        Ptr<Node> P3 = Names::Find<Node>("P3");
        Ptr<Node> P4 = Names::Find<Node>("P4");

        ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPconZipf");
        consumerHelper.SetAttribute("Dsz", UintegerValue(8696));
        consumerHelper.SetAttribute("NumberOfContents", UintegerValue(100000));


        // C1 => EF => 固定速率 1Mbps
        consumerHelper.SetPrefix("/A");
        consumerHelper.SetAttribute("TimingStop", IntegerValue(25000));
        consumerHelper.Install("C1");

        consumerHelper.SetAttribute("TimingStop", IntegerValue(-1));

        // C2 => AF1
        consumerHelper.SetPrefix("/B");
        consumerHelper.SetAttribute("DelayStart", UintegerValue(5000));  // 延迟5秒启动
        consumerHelper.Install("C2");
        

        // C3 => BE
        consumerHelper.SetPrefix("/C");
        consumerHelper.SetAttribute("DelayStart", UintegerValue(15000));  // 延迟10秒启动
        consumerHelper.Install("C3");


        ndn::AppHelper producerHelper("ns3::ndn::Producer");
        producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

        // Register /dst1 prefix with global routing controller and
        // install producer that will satisfy Interests in /dst1 namespace
        // P1
        ndnGlobalRoutingHelper.AddOrigins("/A", P2);
        ndnGlobalRoutingHelper.AddOrigins("/B", P3);
        ndnGlobalRoutingHelper.AddOrigins("/C", P4);
        ndnGlobalRoutingHelper.AddOrigins("/A", P1);
        ndnGlobalRoutingHelper.AddOrigins("/B", P1);
        ndnGlobalRoutingHelper.AddOrigins("/C", P1);

        producerHelper.SetPrefix("/A");
        producerHelper.Install(P1);
        producerHelper.Install(P2);

        producerHelper.SetPrefix("/B");
        producerHelper.Install(P1);
        producerHelper.Install(P3);
        
        producerHelper.SetPrefix("/C");
        producerHelper.Install(P1);
        producerHelper.Install(P4);

        // Calculate all possible routes and install FIBs
        // ndn::GlobalRoutingHelper::CalculateRoutes();

        ndn::FibHelper::AddRoute("C1", "/A", "R1", 0);
        ndn::FibHelper::AddRoute("C2", "/B", "R1", 0);
        ndn::FibHelper::AddRoute("C3", "/C", "R1", 0);

        ndn::FibHelper::AddRoute("R1", "/A", "R2", 0);
        ndn::FibHelper::AddRoute("R1", "/B", "R2", 0);
        ndn::FibHelper::AddRoute("R1", "/C", "R2", 0);

        ndn::FibHelper::AddRoute("R1", "/A", "R3", 0);
        ndn::FibHelper::AddRoute("R1", "/B", "R3", 0);
        ndn::FibHelper::AddRoute("R1", "/C", "R3", 0);

        ndn::FibHelper::AddRoute("R2", "/A", "P1", 0);
        ndn::FibHelper::AddRoute("R2", "/B", "P1", 0);
        ndn::FibHelper::AddRoute("R2", "/C", "P1", 0);

        ndn::FibHelper::AddRoute("R3", "/A", "P2", 0);
        ndn::FibHelper::AddRoute("R3", "/B", "P3", 0);
        ndn::FibHelper::AddRoute("R3", "/C", "P4", 0);


        Simulator::Stop(Seconds(120));

        ndn::L3RateTracer::InstallAll("dsccp2_throughput.txt", Seconds(1));
        ndn::AppDelayTracer::InstallAll("dsccp2_delay.txt");
        L2RateTracer::InstallAll("dsccp2_drop.txt", Seconds(1));
        Simulator::Run();
        Simulator::Destroy();

        return 0;
    }

} // namespace ns3

int main(int argc, char *argv[])
{
    return ns3::main(argc, argv);
}
