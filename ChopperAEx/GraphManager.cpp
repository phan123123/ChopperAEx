#include <iostream>

#include <queue>

#include "llvm/IR/Function.h"

#include "GraphManager.h"

#include "llvm/IR/Module.h"

#include "llvm/IR/ValueSymbolTable.h"

#include "llvm/IR/Value.h"

#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/Function.h"

#include <llvm/Support/DebugLoc.h>

#include <llvm/DebugInfo.h>

#include <llvm/ADT/SCCIterator.h>

#include "Define.h"

GraphManager::GraphManager(llvm::CallGraph & CG): cg(CG) {
    root = std::make_shared < ExtendedCGNode > (CG.getRoot(), nullptr);
}

std::shared_ptr < ExtendedCGNode > GraphManager::findTargetNode(std::string targetFunction,
    const std::vector < std::string > & allFunctions) {
    target = targetFunction;
    std::vector < std::string > path;

    std::unordered_set < std::shared_ptr < ExtendedCGNode > , ExtendedNodeHasher, ExtendedNodeEq > visited;
    std::queue < std::shared_ptr < ExtendedCGNode >> nodeQueue;

    std::shared_ptr < ExtendedCGNode > target = nullptr;

    // Initial set-up for the root node
    nodeQueue.push(root);

    while (!nodeQueue.empty()) {
        auto current = nodeQueue.front();
        nodeQueue.pop();

        std::string fnName = current -> getFnName();

        if (std::find(allFunctions.begin(), allFunctions.end(), fnName) != allFunctions.end()) {
            skippableFunctions.push_back(fnName);
        }

        // If node containing target function is in queue, we can get the path through it's predecessors.
        if (fnName == targetFunction) {
            target = current;
        }

        // If not visited, add to visited set and run the algorithm
        if (visited.find(current) == visited.end()) {
            visited.insert(current);

            // Iterate through the succesors of the node and add them to the queue
            for (llvm::CallGraphNode::const_iterator it = current -> node -> begin(); it != current -> node -> end(); it++) {
                llvm::CallGraphNode::CallRecord callRecord = * it;
                auto succ = callRecord.second;

                auto extendedSucc = std::make_shared < ExtendedCGNode > (succ, current);
                nodeQueue.push(extendedSucc);
            }
        }
    }

    return target;
}

/* Store shortest path */
void GraphManager::getShortestPath(std::shared_ptr < ExtendedCGNode > target) {
    std::vector < std::shared_ptr < ExtendedCGNode >> path;

    if (!target) {
        shortestPath = path;
        return;
    }

    auto current = target;

    while (current -> pred != nullptr) {
        path.push_back(current);
        current = current -> pred;
    }

    // Also add main to path
    path.push_back(current);

    shortestPath = path;
}

/* Prints the shortest path */
void GraphManager::printShortestPath() {
    if (shortestPath.empty()) {
        std::cout << "Function unreachable from main!\n";
        return;
    }

    std::cout << "Shortest path: ";
    for (auto rit = shortestPath.crbegin(); rit != shortestPath.crend(); rit++) {
        std::cout << ( * rit) -> getFnName() << " ";
    }
    std::cout << std::endl;
}

bool GraphManager::shortestPathContains(std::string fnName) {
    for (auto shortestPathNode: shortestPath) {
        if (shortestPathNode -> getFnName() == fnName) {
            return true;
        }
    }
    return false;
}

/* Skip everything that is not on the shortest path */
void GraphManager::excludeAll() {
    std::unordered_set < std::string > skip;

    for (auto fnName: skippableFunctions) {
        if (fnName == "null") {
            continue;
        }

        if (shortestPathContains(fnName)) {
            continue;
        }

        skip.insert(fnName);
    }

    for (auto fnName: skip) {
        std::cout << fnName << "\n";
    }
}

void GraphManager::insertExclude(std::string fName, int line){
    for(auto &element: excludeFunctions){
        if(fName == element.first){         //if function exist
            element.second.push_back(line);
            return;
        }
    }
    std::vector<int> v;
    v.push_back(line);
    excludeFunctions.push_back(std::make_pair(fName, v));
}

void GraphManager::printResult(){
    for(auto &element : excludeFunctions){
        std::cout << element.first << ":";
        auto v = element.second;
        for (auto &i : v){
            if (&i != &v.front())
                std::cout << "/";
            std::cout << i;
        }
        if(&element != &excludeFunctions.back()){
            //std::cout << ",";
            std::cout << ",";
        }
    }
}

bool GraphManager::checkNormalFunction(std::string fName)
{
    for(std::string i : normalFunctions){
        if(fName == i){
            return true;
        }
    }
    return false;
}

bool GraphManager::checkValuableFunction(llvm::Function *F){
    // not functions
    if(F->getName().startswith("llvm"))
        return false;
    std::string fName = F->getName().str();
    //check normal functions
    if (checkNormalFunction(fName))
        return false;
    return true;
}

int GraphManager::cyclomaticComplexity(llvm::Function *F){
    if ( F->getName().str() == target)
        return -999999;
    auto iteratorStatus = statusMap.find(F);
    auto iteratorComplex = complexMap.find(F);
    bool exist = true;
    if (iteratorStatus == statusMap.end()){
        statusMap.insert(std::make_pair(F, HOLD));
        complexMap.insert(std::make_pair(F, 0));
        exist = false;
    }
    else if(iteratorStatus->second == HOLD){
        return 1;
    }
    else if(iteratorStatus->second == DONE){
        return iteratorComplex->second;
    }

    // cyclonmatic complexity in internal function
    int result = 0;
    for (llvm::Function::const_iterator I = F->begin(), IE = F->end(); I != IE; ++I) {
        colorMap.insert(std::make_pair(&*I,WHITE));
    }
    for (auto &bb: *F){
        result += blockCyclomaticComplexity(&bb);
    }
    colorMap.clear();
    
    // plus more from called function
    for (auto &bb: *F){
        for (auto &instruction : bb){
            if (llvm::CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(&instruction)) {
                if (llvm::Function *calledFunction = callInst->getCalledFunction()){
                    if (!checkValuableFunction(calledFunction))
                        continue; 
                    result += cyclomaticComplexity(calledFunction);                    
                }
            }
        }
    }
    iteratorStatus = statusMap.find(F);
    iteratorComplex = complexMap.find(F);  
    iteratorStatus->second = DONE;
    iteratorComplex->second = result;
//    llvm::outs() << "\n" << F->getName() << " +++ " << result << "\n";
    return result;
}

int GraphManager::blockCyclomaticComplexity(const llvm::BasicBlock *BB){
    int result =0;
    colorMap.find(BB)->second = BLACK; 
    const llvm::TerminatorInst *TInst = BB->getTerminator();
    for (unsigned I = 0, NSucc = TInst->getNumSuccessors(); I < NSucc; ++I) {
        llvm::BasicBlock *Succ = TInst->getSuccessor(I);
        auto succIterator = colorMap.find(Succ);
        Color succColor = succIterator->second; 
        if (succColor == WHITE){
            succIterator->second = BLACK;
        }
        else{
            result += 1;
        }
    }
    return result;
}

void GraphManager::excludeSelective() {
    for (auto rit = shortestPath.crbegin(); rit != shortestPath.crend(); rit++) {
        auto checkEnd = rit + 1;
        if (checkEnd == shortestPath.crend())
            break;
        //std::cout << "Function: " << ( * rit) -> getFnName() << std::endl;
	    llvm::Function *F = ((*rit)->node->getFunction());
        for (auto &bb: *((*rit)->node->getFunction())){
            for (auto &instruction : bb){
                if (llvm::CallInst *callInst = llvm::dyn_cast<llvm::CallInst>(&instruction)) {
                    if (llvm::Function *calledFunction = callInst->getCalledFunction()){
                        
                        std::string fName = calledFunction->getName().str();
                        if (!checkValuableFunction(calledFunction))
                            continue;
			    //check in shortest path
                        if (shortestPathContains(fName))
                            continue;
                        // detect make symbolic function and clear exclude functions vector
                        if(fName == sigFunction){
                            excludeFunctions.clear(); 
                            continue;
                        }
                        //std::cout << fName << " --- " << cyclomaticComplexity(calledFunction) << "\n";
                        if(cyclomaticComplexity(calledFunction) +1 < MIN_COMPLEXITY)
                            continue;
                        
                        //choiced
                        const llvm::DebugLoc &debugInfo = instruction.getDebugLoc();
                        int line = debugInfo.getLine();
                        insertExclude(fName,line);
                    }
                }
            }
        }
    }
    printResult();
}
