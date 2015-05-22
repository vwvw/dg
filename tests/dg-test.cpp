#include <assert.h>
#include <cstdarg>
#include <cstdio>

#include "test-runner.h"

#include "../src/DependenceGraph.h"
#include "../src/EdgesContainer.h"

namespace dg {
namespace tests {

class TestDG;
class TestNode;

class TestNode : public Node<TestDG, const char *, TestNode *>
{
public:
    TestNode(const char *name)
        : Node<TestDG, const char *, TestNode *>(name) {}

    const char *getName() const { return getKey(); }
};

class TestDG : public DependenceGraph<const char *, TestNode *>
{
public:
#ifdef ENABLE_CFG
    typedef BBlock<TestNode *> BasicBlock;
#endif // ENABLE_CFG

    bool addNode(TestNode *n)
    {
        return DependenceGraph<const char *, TestNode *>
                ::addNode(n->getName(), n);
    }
};

#define CREATE_NODE(n) TestNode n(#n)

class TestConstructors : public Test
{
    TestConstructors() : Test("constructors test") {}

    void test()
    {
        TestDG d;

        check(d.getEntry() == nullptr, "BUG: garbage in entry");
        check(d.getSize() == 0, "BUG: garbage in nodes_num");

        //TestNode n;
        CREATE_NODE(n);

        check(!n.hasSubgraphs(), "BUG: garbage in subgraph");
        check(n.subgraphsNum() == 0, "BUG: garbage in subgraph");
        check(n.getParameters() == nullptr, "BUG: garbage in parameters");
    }
};

class TestAdd : public Test
{
public:
    TestAdd() : Test("edges adding test")
    {}

    void test()
    {
        TestDG d;
        //TestNode n1, n2;
        CREATE_NODE(n1);
        CREATE_NODE(n2);

        check(n1.addControlDependence(&n2), "adding C edge claims it is there");
        check(n2.addDataDependence(&n1), "adding D edge claims it is there");

        d.addNode(&n1);
        d.addNode(&n2);

        d.setEntry(&n1);
        check(d.getEntry() == &n1, "BUG: Entry setter");

        int n = 0;
        for (auto I = d.begin(), E = d.end(); I != E; ++I) {
            ++n;
            check((*I).second == &n1 || (*I).second == &n2,
                    "Got some garbage in nodes");
        }

        check(n == 2, "BUG: adding nodes to graph, got %d instead of 2", n);

        int nn = 0;
        for (auto ni = n1.control_begin(), ne = n1.control_end();
             ni != ne; ++ni){
            check(*ni == &n2, "got wrong control edge");
            ++nn;
        }

        check(nn == 1, "bug: adding control edges, has %d instead of 1", nn);

        nn = 0;
        for (auto ni = n2.data_begin(), ne = n2.data_end();
             ni != ne; ++ni) {
            check(*ni == &n1, "got wrong control edge");
            ++nn;
        }

        check(nn == 1, "BUG: adding dep edges, has %d instead of 1", nn);
        check(d.getSize() == 2, "BUG: wrong nodes num");

        // adding the same node should not increase number of nodes
        check(!d.addNode(&n1), "should get false when adding same node");
        check(d.getSize() == 2, "BUG: wrong nodes num (2)");
        check(!d.addNode(&n2), "should get false when adding same node (2)");
        check(d.getSize() == 2, "BUG: wrong nodes num (2)");

        // don't trust just the counter
        n = 0;
        for (auto I = d.begin(), E = d.end(); I != E; ++I)
            ++n;

        check(n == 2, "BUG: wrong number of nodes in graph", n);

        // we're not a multi-graph, each edge is there only once
        // try add multiple edges
        check(!n1.addControlDependence(&n2),
             "adding multiple C edge claims it is not there");
        check(!n2.addDataDependence(&n1),
             "adding multiple D edge claims it is not there");

        nn = 0;
        for (auto ni = n1.control_begin(), ne = n1.control_end(); ni != ne; ++ni){
            check(*ni == &n2, "got wrong control edge (2)");
            ++nn;
        }

        check(nn == 1, "bug: adding control edges, has %d instead of 1 (2)", nn);

        nn = 0;
        for (auto ni = n2.data_begin(), ne = n2.data_end();
             ni != ne; ++ni) {
            check(*ni == &n1, "got wrong control edge (2) ");
            ++nn;
        }

        check(nn == 1,
                "bug: adding dependence edges, has %d instead of 1 (2)", nn);
    }
};

class TestContainer : public Test
{
public:
    TestContainer() : Test("container test")
    {}

    void test()
    {
#if ENABLE_CFG
        CREATE_NODE(n1);
        CREATE_NODE(n2);

        EdgesContainer<TestNode *> IT;
        EdgesContainer<TestNode *> IT2;

        check(IT == IT2, "empty containers does not equal");
        check(IT.insert(&n1), "returned false with new element");
        check(IT.size() == 1, "size() bug");
        check(IT2.size() == 0, "size() bug");
        check(IT != IT2, "different containers equal");
        check(IT2.insert(&n1), "returned false with new element");
        check(IT == IT2, "containers with same content does not equal");

        check(!IT.insert(&n1), "double inserted element");
        check(IT.insert(&n2), "unique element wrong retval");
        check(IT2.insert(&n2), "unique element wrong retval");

        check(IT == IT2, "containers with same content does not equal");
#endif
    }
};


class TestCFG : public Test
{
public:
    TestCFG() : Test("CFG edges test")
    {}

    void test()
    {
#if ENABLE_CFG

        TestDG d;
        CREATE_NODE(n1);
        CREATE_NODE(n2);

        d.addNode(&n1);
        d.addNode(&n2);

        check(!n1.hasSuccessor(),
                "hasSuccessor returned true on node without successor");
        check(!n2.hasSuccessor(),
                "hasSuccessor returned true on node without successor");
        check(!n1.hasPredcessor(),
                "hasPredcessor returned true on node without successor");
        check(!n2.hasPredcessor(),
                "hasPredcessor returned true on node without successor");
        check(n1.getSuccessor() == nullptr, "succ initialized with garbage");
        check(n2.getSuccessor() == nullptr, "succ initialized with garbage");
        check(n1.getPredcessor() == nullptr, "pred initialized with garbage");
        check(n2.getPredcessor() == nullptr, "pred initialized with garbage");

        check(n1.addSuccessor(&n2) == nullptr,
                "adding successor edge claims it is there");
        check(n1.hasSuccessor(), "hasSuccessor returned false");
        check(!n1.hasPredcessor(), "hasPredcessor returned true");
        check(n2.hasPredcessor(), "hasPredcessor returned false");
        check(!n2.hasSuccessor(), "hasSuccessor returned false");
        check(n1.getSuccessor() == &n2, "get/addSuccessor bug");
        check(n2.getPredcessor() == &n1, "get/addPredcessor bug");

        // basic blocks
        TestDG::BasicBlock BB(&n1);
        check(BB.getFirstNode() == &n1, "first node incorrectly set");
        check(BB.setLastNode(&n2) == nullptr, "garbage in lastNode");
        check(BB.getLastNode() == &n2, "bug in setLastNode");

        check(BB.successorsNum() == 0, "claims: %u", BB.successorsNum());
        check(BB.predcessorsNum() == 0, "claims: %u", BB.predcessorsNum());

        CREATE_NODE(n3);
        CREATE_NODE(n4);
        d.addNode(&n3);
        d.addNode(&n4);

        TestDG::BasicBlock BB2(&n3), BB3(&n3);

        check(BB.addSuccessor(&BB2), "the edge is there");
        check(!BB.addSuccessor(&BB2), "added even when the edge is there");
        check(BB.addSuccessor(&BB3), "the edge is there");
        check(BB.successorsNum() == 2, "claims: %u", BB.successorsNum());

        check(BB2.predcessorsNum() == 1, "claims: %u", BB2.predcessorsNum());
        check(BB3.predcessorsNum() == 1, "claims: %u", BB3.predcessorsNum());
        check(*(BB2.predcessors().begin()) == &BB, "wrong predcessor set");
        check(*(BB3.predcessors().begin()) == &BB, "wrong predcessor set");

        for (auto s : BB.successors())
            check(s == &BB2 || s == &BB3, "Wrong succ set");

        BB2.removePredcessors();
        check(BB.successorsNum() == 1, "claims: %u", BB.successorsNum());
        check(BB2.predcessorsNum() == 0, "has successors after removing");

        BB.removeSuccessors();
        check(BB.successorsNum() == 0, "has successors after removing");
        check(BB2.predcessorsNum() == 0, "removeSuccessors did not removed BB"
                                        " from predcessor");
        check(BB3.predcessorsNum() == 0, "removeSuccessors did not removed BB"
                                    " from predcessor");
#endif // ENABLE_CFG
    }
};

}; // namespace tests
}; // namespace dg

int main(int argc, char *argv[])
{
    using namespace dg::tests;
    TestRunner Runner;

    Runner.add(new TestCFG());
    Runner.add(new TestContainer());
    Runner.add(new TestAdd());

    return Runner();
}
