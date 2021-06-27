#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <unordered_set>

#include <unordered_map>

#include <llvm/Analysis/CFG.h>

#include "llvm/Analysis/CallGraph.h"

#include "llvm/Analysis/CallGraphSCCPass.h"

#include "ExtendedCGNode.h"

#include <llvm/ADT/DenseMap.h>

/* color to mark to basic block when calculate cyclomatic comlexity of function
WHITE: still not visit
BLACK: visited
*/
enum Color {WHITE, BLACK};

/*status of function when calculate cyclomatic complexity
DONE: calculated
HOLD: in process
FREE: still not calculate
*/
enum Status {DONE, HOLD, FREE};

#define MIN_COMPLEXITY 5

class GraphManager {
    public:
        GraphManager(llvm::CallGraph & CG);
    std::shared_ptr < ExtendedCGNode > findTargetNode(std::string targetFunction,
        const std::vector < std::string > & );
        void getShortestPath(std::shared_ptr < ExtendedCGNode > target);
        bool shortestPathContains(std::string fnName);
        void printShortestPath();
        void excludeAll();
        void excludeSelective();
    private:
        llvm::CallGraph & cg;
        std::string target;
        std::shared_ptr < ExtendedCGNode > root;
        std::vector < std::shared_ptr < ExtendedCGNode >> shortestPath;
        std::vector < std::string > skippableFunctions;
        std::vector <std::pair<std::string, std::vector<int>>> excludeFunctions;
        void insertExclude(std::string fName, int line);
        void printResult();
        bool checkNormalFunction(std::string fName);
        bool checkValuableFunction(llvm::Function *F);

        std::unordered_map<const llvm::BasicBlock *, Color>colorMap;
        std::unordered_map<const llvm::Function *, int> complexMap;
        std::unordered_map<const llvm::Function *, Status> statusMap;
	    int cyclomaticComplexity(llvm::Function *F);
        int blockCyclomaticComplexity(const llvm::BasicBlock *BB);
        std::string sigFunction = "klee_make_symbolic";
};

#endif /* GRAPHMANAGER_H */
