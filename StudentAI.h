#ifndef STUDENTAI_H
#define STUDENTAI_H
#include "AI.h"
#include "Board.h"
#pragma once

//The following part should be completed by students.
//Students can modify anything except the class name and exisiting functions and varibles.
class StudentAI :public AI
{
public:
    Board board;
	StudentAI(int col, int row, int p);
	virtual Move GetMove(Move board);
	int captureCount(Board, int);
	vector<Move> filter_moves(Board, int);

// added for runtimes
private:
	double totalRuntime = 0.0; // tracks cumulative runtime
	int numMoves = 0; // tracks AI's numMoves --> also used to compute avg runtime later on
	bool becomesKing(const Board&, const Move&, int);
};

#endif //STUDENTAI_H