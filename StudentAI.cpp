#include "StudentAI.h"
#include "Board.h"
#include <random>
#include <limits> // computeUCT (sets numeric_limit for unvisited nodes)
#include <cmath> // computeUCT (log, sqrt, etc.)
#include <chrono> // runtime tracker


using namespace std;

// represents a single state in MCTS
// stores info regarding state: the move leading up to it, its parent-child relationships, any relavant statistical data
struct Node{ 
    Board currState; // curr state of game at this node
    Move parentMove; // parent's move that led to this node
    Node* parent; // previous game state
    vector<Node*> children; // represents list of possible moves from this state

    int numVisits; // tracks num of times this node has been visited during MCTS
    int player; // represents player (0 or 1) who made the move leading to this node
    double wins; // tracks num of wins from simulations passing through this node --> used to compute win rate

    Node(Board currState, Move parentMove, Node* parent, int player)
        : currState(currState), parentMove(parentMove), parent(parent), numVisits(0), wins(0.0), player(player) {}

    ~Node(){
        for(Node* child: children){
            delete child;
        }
    }

    // MCTS's selection step balances exploration and exploitation, uses this UCT formula
    // UCT = (wins/visits) + C * (sqrt(ln parent.visits/visits))
        // wins/vists --> exploitation --> win rate
        // C * (sqrt(ln parent.visits/visits)) --> exploration
    double computeUCT(double exploration = 1.4)const{
        if (parent == nullptr) return 1000;
        if(numVisits == 0  && parent->parent == nullptr) return 999; //numeric_limits<double>::infinity(); // set unvisited node's UCT value to infinity to encourage exploration/selection
        //cout << "Wins: " << wins << endl;
        return (wins/numVisits) + exploration * sqrt(log(parent->numVisits)/numVisits);
    }

};


class MCTS{
    public:
        // MCTS constructor
        MCTS(Board currState, int player, int iters = 2) {
            //constructing tree
            //cout << "Constructing tree" << endl;
            this->player = player;
            this->iters = iters;
            root = new Node(currState, Move(), nullptr, player);
            for (unsigned int i = 1; i <= 1000; ++i) {
                // cout << "ITERATION " << i << endl;
                // cout << "Children in root: " << root->children.size() << endl;
                Node* selectNode = selection(root);
                // cout << "SELECT NODE" << endl;
                // cout << selectNode->computeUCT() << endl;
                // selectNode->currState.showBoard();
                Node* expandNode = expansion(selectNode);
                // cout << "EXPAND NODE" << endl;
                // expandNode->currState.showBoard();
                // cout << "Expanded node's parent and player: " << expandNode->parent->player << " Move: " << expandNode->parentMove.toString() << endl;
                // expandNode->parent->currState.showBoard();
                int result = simulation(expandNode->currState, expandNode->player);
                // cout << "RESULT: " << result << endl;
                backpropagation(expandNode, result);
                // cout << endl;
                // cout << endl;
            }
            // cout << "POSSIBLE MOVES: " << root->children.size() << endl;
        }

        // MCTS recursive destructor
        void MCTS_destructor(Node* currNode) {
            for (unsigned int i = 0; i < currNode->children.size(); ++i) {
                MCTS_destructor(currNode->children[i]);
            }
            if (!currNode) {
                //cout << "deleting" << endl;
                delete currNode;
                currNode = nullptr;
            }
        }

        // MCTS destructor
        ~MCTS() {
            MCTS_destructor(root);
        }

        Move search() {
            // cout << "starting search, should call selection, expansion, simulation, backpropagation, and chosen move \n" << endl;

            return chosenMove();
            // cout << "completed search \n" << endl;
        }

        Move mcts_safeMoves(vector<Move> safeMoves, string curr_res) {
            Node* maxChild = nullptr;
            int maxWins = -1;
            for (auto& safeMove : safeMoves) {
                for (auto& child : root->children) {
                    if (safeMove.toString() == child->parentMove.toString() && child->parentMove.toString() != curr_res && child->wins > maxWins) {
                        maxChild = child;
                        maxWins = maxChild->wins;
                    }
                }
            }
            if (maxChild == nullptr) {
                return Move(curr_res);
            }
            return maxChild->parentMove;
        }

    private:
        int player;
        int iters;
        Node* root;

        Node* selection(Node* currNode) {
            // cout << "starting selection \n" << endl;
            Node* maxChild = currNode;
            // currNode->currState.showBoard();
            // cout << currNode->currState.isWin(player) << endl;

            //possibleMoves
            //cout << "PLAYER: " << player << endl;
            vector<vector<Move>> possibleMoves = root->currState.getAllPossibleMoves(player);
            unsigned movesCount = 0;
            //cout << "possibleMoves" << endl;
            for (auto& piece : possibleMoves) {
                for (auto& move : piece) {
                    //cout << move.toString() << endl;
                    movesCount++;
                }
            }

            while (!currNode->children.empty() && currNode->currState.isWin(player) == 0 && root->children.size() == movesCount) {
                // maxChild = child with the greatest UCT value
                // initialize maxChild with first child
                maxChild = currNode->children[0];
                for (unsigned int i = 1; i < currNode->children.size(); ++i) {
                    if (currNode->children[i]->computeUCT() > maxChild->computeUCT()) {
                        maxChild = currNode->children[i];
                    }
                }
                currNode = maxChild;
            }
            // cout << "completed selection \n" << endl;
            // cout << "MAXCHILD should be root" << endl;
            // maxChild->currState.showBoard();
            // if (maxChild == root) {
            //     cout << "SELECT THE ROOT" << endl;
            // }
            // else {
            //     cout << "MAX CHILD'S UCT: " << maxChild->computeUCT() << endl;
            //     // if (maxChild->currState.isWin(player) != 0) {
            //     //     cout << "RETURNING PARENT" << endl;
            //     //     return maxChild->parent;
            //     // }
            // }
            return maxChild;
        }

        Node *expansion(Node* currNode) {
            // cout << "starting expansion \n" << endl;
            // get all possible moves from the currNode
            // root is board after opponent just made move, children of root is ai's move
            vector<vector<Move>> possibleMoves = currNode->currState.getAllPossibleMoves(currNode->player);
            
            //if there are no more possible moves, then terminal node. do not expand
            if (possibleMoves.empty()) {
                // cout << "completed expansion - no possible moves (don't expand) \n" << endl;
                return currNode;
            }
            
            // get untried moves
            vector<Move> untriedMoves;

            // iterate through possible moves to get untried moves
            for (unsigned int i = 0; i < possibleMoves.size(); ++i) {
                for (unsigned int j = 0; j < possibleMoves[i].size(); ++j) {
                    bool moveTried = false;
                    for (Node* currChild : currNode->children) {
                        // if equal to parent move, then this move was already tried to get to this node
                        if (currChild->parentMove.toString() == possibleMoves[i][j].toString()) {
                            moveTried = true;
                            break;
                        }
                    }
                    if (moveTried == false) {
                        untriedMoves.push_back(possibleMoves[i][j]);
                    }
                }
            }

            if (!untriedMoves.empty()) {
                Move randomMove = untriedMoves[rand() % untriedMoves.size()];
                //add corresponding child node to tree
                //cout << untriedMove.toString() << endl;
                Board newState = currNode->currState;
                newState.makeMove(randomMove, currNode->player);
                currNode->children.push_back(new Node(newState, randomMove, currNode, 3 - currNode->player));
                // return the new child expanded from currNode
                return currNode->children.back();
            }

            // // cout << "completed expansion - no untried moves \n" << endl;
            // // if there are no untried moves, return currNode because no expansion
            return currNode;
        }

        // simulation: random playout from given game state until the end, returns winner (player 1 or 2)
        int simulation(Board simulatedState, int simulatedPlayer){
            // cout << "starting simulation \n" << endl;
            //cout << "Starting with player: " << simulatedPlayer << endl;
            while(simulatedState.isWin(simulatedPlayer) == 0){ // runs game until there's a winner
                vector<vector<Move>> legalMoves = simulatedState.getAllPossibleMoves(simulatedPlayer);
                // cout << "possibleMoves" << endl;
                // for (auto& piece : legalMoves) {
                //     for (auto& move : piece) {
                //         cout << move.toString() << endl;
                //     }
                // }
                if(!legalMoves.empty()) {
                    //cout << "Legal Moves not empty" << endl;
                    vector<Move> randomMoves = legalMoves[rand() % legalMoves.size()]; // select a random group of next moves
                    Move nextMove = randomMoves[rand() % randomMoves.size()]; // from that group, select ONE random next move
                
                    // execute the chosen move on the current simulated state and switch to other player's turn
                    simulatedState.makeMove(nextMove, simulatedPlayer);
                    //cout << "Made move: " << nextMove.toString() << endl;
                    simulatedPlayer = 3 - simulatedPlayer;
                    //cout << "Now player: " << simulatedPlayer << endl;

                } else {
                    //cout << "completed simulation - no possible moves, other player wins \n" << endl;
                    return 3 - simulatedPlayer; // if no possible moves, the other player wins
                }
            }
            // cout << "completed simulation \n";
            // cout << simulatedState.isWin(3 - simulatedPlayer) << endl;
            // cout << "Final player: " << simulatedPlayer << endl;
            // simulatedState.showBoard();
            return simulatedState.isWin(3 -simulatedPlayer); // returns winner
        }

        // backpropagation: update statistics (wins, numVisits) for all nodes along the tried path
        void backpropagation(Node* node, int result){ // result is the simulation's winner (player 1 or 2)
            // cout << "starting backpropagation \n" << endl;
            // start w/ leaf node where the simulation ended, continue until root is reached
            while(node){
                if(node->player == 3 - result){
                    node->wins++;
                }
                node->numVisits++;
                node = node->parent;
            }
            // cout << "completed backpropagation \n" << endl;
        }

        // chooses the best move after MCTS
        Move chosenMove(){
            // cout << "starting chosenMove \n" << endl;
            Node* chosenNode = nullptr; // tracks the best move
            int maxVisits = -1; 
            double maxUCT = -1;

            // iterate through root node's children, find node with highest num of visits (explored more thoroughly in simulations --> more reliable choice)
            // choose node with maxVisits --> helps balance exploitation of winning states + exploration of new states --> UCT 
            for(Node* child : root->children){
                // cout << child->computeUCT() << endl;
                // cout << child->player << endl;
                // child->currState.showBoard();
                if (maxVisits == child->numVisits) {
                    if (child->computeUCT() > maxUCT) {
                        chosenNode = child;
                        maxUCT = child->computeUCT();
                    }
                }
                if(maxVisits < child->numVisits){
                    maxVisits = child->numVisits;
                    maxUCT = child->computeUCT();
                    chosenNode = child;
                }
            }
            
            // cout << "completed chosenMove \n" << endl;
            return chosenNode->parentMove;
        }

};

int StudentAI::captureCount(Board board, int opp_player) {
    int count = 0;
    vector<vector<Move>> possible_moves = board.getAllPossibleMoves(opp_player);

    for (auto& piece_moves : possible_moves) {
        for (auto& move : piece_moves) {
            if (move.isCapture()) {
                count++;
            }
        }
    }

    return count;
}

// helper function to check if a move results in a king
bool StudentAI::becomesKing(const Board& board, const Move& move, int player){
    // create a copy of the board and make a move
    Board copyBoard = board;
    copyBoard.makeMove(move, player);

    // gets last position in move sequence
    const Position& lastPos = move.seq.back();

    // access row (x) and column (y)
    int row = lastPos.x;
    int col = lastPos.y;

    // check if piece at last position is a king
    const Checker& piece = copyBoard.board[row][col];
    return piece.isKing;
}

vector<Move> StudentAI::filter_moves(Board board, int player) {
    vector<Move> result;
    map<int, Move> captures;
    vector<vector<Move>> my_moves = board.getAllPossibleMoves(player);
    int score = 0;
    for (auto& piece : my_moves) {
        for (auto& move : piece) {
            //cout << move.toString() << endl;
            //cout << player << endl;
            board.makeMove(move, player);
            score = captureCount(board, 3 - player);
            board.Undo();
            if (score > 0) {
                captures[score] = move;
            }
            else {
                result.push_back(move);
            }
        }
    }
    if (result.empty()) {
        auto smallest_score = captures.begin();
        result.push_back(smallest_score->second); 
    } else{
        // favor king-making moves in case of ties
        vector<Move> king_moves;
        for(auto& move : result){
            if(becomesKing(board, move, player)){
                king_moves.push_back(move);
            }
        }
        if(!king_moves.empty()){
            result = king_moves;
        }
    }
    return result;
}

//The following part should be completed by students.
//The students can modify anything except the class name and exisiting functions and varibles.
StudentAI::StudentAI(int col,int row,int p)
	:AI(col, row, p), numMoves(0)
{
    board = Board(col,row,p);
    board.initializeGame();
    player = 2;
}

Move StudentAI::GetMove(Move move)
{
    if (move.seq.empty())
    {
        player = 1;
    } else{
        board.makeMove(move,player == 1?2:1);
    }

    srand(time(0));

    // cout << "Cumulative: runtime " << totalRuntime << " seconds\n";
    if (totalRuntime > 480.0){
        // cout << "Warning: Exceeded 8-minute time limit!\n";
        vector<vector<Move> > moves = board.getAllPossibleMoves(player);
        int i = rand() % (moves.size());
        vector<Move> checker_moves = moves[i];
        int j = rand() % (checker_moves.size());
        Move res = checker_moves[j];
        board.makeMove(res,player);
        return res;
    }

    // increment numMoves for each AI move
    numMoves++;

    // start timer
    auto start = chrono::high_resolution_clock::now();

    // vector<vector<Move>> possibleMoves = board.getAllPossibleMoves(player);

    //possibleMoves
    // cout << "possibleMoves" << endl;
    // for (auto& piece : possibleMoves) {
    //     for (auto& move : piece) {
    //         cout << move.toString() << endl;
    //     }
    // }

    //cout << "PLAYER: " << player << endl;
    MCTS mcts(board, player, 1000);
    Move res = mcts.search();

    // CHECK FOR CAPTURES

    vector<vector<Move> > all_moves = board.getAllPossibleMoves(player);
    vector<Move> best_moves;
    bool no_capture = true;
    bool res_is_captured = false;

    //cout << best_move.toString() << endl;

    for (auto& my_piece : all_moves) {
        for (auto& my_move : my_piece) {
            if (my_move.isCapture() || res.toString() == my_move.toString()) {

                board.makeMove(my_move, player);
                vector<vector<Move>> opp_moves = board.getAllPossibleMoves(3 - player);
                bool opp_capture = false;
                for (auto& opp_piece : opp_moves) {
                    for (auto& opp_move : opp_piece) {
                        if (opp_move.isCapture()) {
                            opp_capture = true;
                            res_is_captured = true;
                        }
                    }
                }
                board.Undo();

                if (!opp_capture  && my_move.isCapture()) {
                    best_moves.push_back(my_move);
                    no_capture = false;
                }
            }
        }
    }

    if (!no_capture) {
        //cout << "make a captured move" << endl;
        res = best_moves[rand() % best_moves.size()];
    }
    else if (res_is_captured) {
        //cout << "res is captured" << endl;
        string curr_res = res.toString();
        vector<Move> safe_moves = filter_moves(board, player);
        // cout << "Safe moves" << endl;
        // for (auto& safeMove : safe_moves) {
        //     cout << safeMove.toString() << endl;
        // }
        res = mcts.mcts_safeMoves(safe_moves, curr_res);

    }

    //cout << best_move.toString() << endl;
    // cout << "Best moves" << endl;
    // for (auto& move : best_moves) {
    //     cout << move.toString() << endl;
    // }
    // cout << endl;


    // calculate and add up total runtime 
    auto end = chrono::high_resolution_clock::now();
    double moveTime = chrono::duration<double>(end - start).count();
    totalRuntime += moveTime; // accumulate total runtime
    // cout << "Move took: " << moveTime << " seconds\n";
    
    // cout << "Number of moves so far: " << numMoves << "\n";

    board.makeMove(res, player);
    return res;
}