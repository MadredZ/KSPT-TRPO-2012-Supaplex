#include "Simulator.h"


Simulator::Simulator(Field * amine)
{
	this->mine = amine;
	robotIsDead = false;
}


Simulator::~Simulator(void)
{
}

vector<pair<int, int>> Simulator::GetPath()
{
	return this->path;
}

void Simulator::StartSimulation(vector<pair<int, int>> waypoints)
{
	mine->ClearLambdas();
	for (int i = waypoints.size() - 1; i > 0; i--) {							// waypoint #0 is a Robot !!!
		mine->AddLambda(waypoints.at(i));
	}

	int result;
	for (int i = mine->GetLambdas().size() - 1; i > 0; i--) {					// waypoint #0 is a Robot !!!
		result = MoveRobot(mine->GetLambdas().at(i));
		if (result == -1) break;											// ����� �� break - ��������

		mine->GetLambdas().pop_back();
	}
}

// Description: Updates map according to the rules
void Simulator::UpdateMap()
{
	// Creating new state to record changes on the map
	char ** newState = new char* [mine->GetHeight()];
	for (int i = 0; i < mine->GetHeight(); i++) {
		newState[i] = new char [mine->GetWidth()];
		for (int j = 0; j < mine->GetWidth(); j++) {
			newState[i][j] = '#';
		}
	}
	
	for (int i = 1; i < mine->GetHeight() - 1; i++) {
		for (int j = 1; j < mine->GetWidth() - 1; j++) {
			// If (x; y) contains a Rock, and (x; y-1) is Empty:
			// (x; y) is updated to Empty, (x; y-1) is updated to Rock.
			if (mine->GetMap()[i][j] == '*' && mine->GetMap()[i + 1][j] == ' ') {
				newState[i][j] = ' ';
				newState[i + 1][j] = '*';
				if (mine->GetMap()[i + 2][j] == 'R') robotIsDead = true;
			}
			// If (x; y) contains a Rock, (x; y-1) contains a Rock, (x+1; y) is Empty and (x+1; y-1) is Empty:
			// (x; y) is updated to Empty, (x+1; y-1) is updated to Rock.
			else if (mine->GetMap()[i][j] == '*' && mine->GetMap()[i + 1][j] == '*'
				&& mine->GetMap()[i][j + 1] == ' ' && mine->GetMap()[i + 1][j + 1] == ' ') {
					newState[i][j] = ' ';
					newState[i + 1][j + 1] = '*';
					if (mine->GetMap()[i + 2][j + 1] == 'R') robotIsDead = true;
			}
			// If (x; y) contains a Rock, (x; y-1) contains a Rock, either (x+1; y) is not Empty
			// or (x+1; y-1) is not Empty, (x-1; y) is Empty and (x-1; y-1) is Empty:
			// (x; y) is updated to Empty, (x-1; y-1) is updated to Rock.
			else if (mine->GetMap()[i][j] == '*' && mine->GetMap()[i + 1][j] == '*'
				&& (mine->GetMap()[i][j + 1] != ' ' || mine->GetMap()[i + 1][j + 1] != ' ')
				&& mine->GetMap()[i][j - 1] == ' ' && mine->GetMap()[i + 1][j - 1] == ' ') {
					newState[i][j] = ' ';
					newState[i + 1][j - 1] = '*';
					if (mine->GetMap()[i + 2][j - 1] == 'R') robotIsDead = true;
			}
			// If (x; y) contains a Rock, (x; y-1) contains a Lambda, (x+1; y) is Empty and (x+1; y-1) is Empty:
			// (x; y) is updated to Empty, (x+1; y-1) is updated to Rock.
			else if (mine->GetMap()[i][j] == '*' && mine->GetMap()[i + 1][j] == '\\'
				&& mine->GetMap()[i][j + 1] == ' ' && mine->GetMap()[i + 1][j + 1] == ' ') {
					newState[i][j] = ' ';
					newState[i + 1][j + 1] = '*';
					if (mine->GetMap()[i + 2][j + 1] == 'R') robotIsDead = true;
			}
			// In all other cases, (x; y) remains unchanged.
		}
	}
	// If (x; y) contains a Closed Lambda Lift, and there are no Lambdas remaining:
	// (x; y) is updated to Open Lambda Lift.
	if (mine->GetLambdas().empty()) {
		mine->SetLiftState(true);
		newState[mine->GetLift().first][mine->GetLift().second] = 'O';
	}

	// Rewriting old map according to the new state
	for (int i = 0; i < mine->GetHeight(); i++) {
		for (int j = 0; j < mine->GetWidth(); j++) {
			if (newState[i][j] != '#') mine->GetMap()[i][j] = newState[i][j];
		}
	}

	// Freeing memory
	for (int i = 0; i < mine->GetHeight(); i++)
		delete [] newState[i];
}

int Simulator::MoveRobot(pair<int, int> target)
{

																// ������������ �*, ���������������� ��� ����� ������������ ������� � �����.
																// ��� ������ - ����������� �� ������ ������
	int time = 0;
	const int infinity = 1000000;
	int startX = mine->GetRobot().first;
	int startY = mine->GetRobot().second;



	vector<pair<int, int>> resultPath;			// vector of coordinates of cells in found path
	int result = 0;
	const int nonexistent = 0, found = 1;		// path-related constants
	const int inOpenList = 1, inClosedList = 2;	// lists-related constants
	int parentX, parentY, Gcost, index;
	int ** whichList;			// used to record whether a cell is on the open list or on the closed list.
	pair<int, int> ** parent;	// used to record parent of each cage
	OpenListItem * openList;	// array holding open list items, which is maintained as a binary heap.
	int numberOfOpenListItems;

// 1. Allocate memory and reset variables

	whichList = new int* [mine->GetHeight() + 1];
	for (int i = 0; i < mine->GetHeight(); i++) {
		whichList[i] = new int [mine->GetWidth() + 1];
		for (int j = 0; j < mine->GetWidth(); j++)
			whichList[i][j] = 0;
	}

	parent = new pair<int, int>* [mine->GetHeight() + 1];
	for (int i = 0; i < mine->GetHeight(); i++) {
		parent[i] = new pair<int, int> [mine->GetWidth() + 1];
	}

	openList = new OpenListItem [mine->GetWidth()*mine->GetHeight()+2];

// 2. Add the starting cell to the open list.

	numberOfOpenListItems = 1;
	openList[1].SetX(startX);	// assign it as the top item in the binary heap
	openList[1].SetY(startY);
	openList[1].SetGcost(0);	// reset starting cell's G value to 0

// 3. Do it until the path is found or recognized as nonexistent.

	while (true) {
	
// 3.1. If the open list is not empty, take the first cell off of the list (i.e. the lowest F cost cell).

		if (numberOfOpenListItems != 0) {
// ***************************
			if (openList[1].GetHcost() == infinity) {
				result = nonexistent;
				break;
			}

			MakeSnapshot();
			
			Step(openList[1].GetX(), openList[1].GetY());
			UpdateMap();
			time++;

			if (robotIsDead) {
				openList[1].SetHcost(infinity);
				SinkItemInBinaryHeap(openList, numberOfOpenListItems, 1);
				LoadSnapshot();
				time--;
			}
// ***************************

			// record cell coordinates and Gcost of the item as parent for adjacent cells (see below)
			parentX = openList[1].GetX();
			parentY = openList[1].GetY();
			Gcost = openList[1].GetGcost();

			whichList[parentX][parentY] = inClosedList;						// add item to the closed list
			DeleteTopItemFromBinaryHeap(openList, numberOfOpenListItems);	// delete this item from the open list

// 4.2. Check the adjacent squares and add them to the open list

			for (int x = parentX - 1; x <= parentX + 1; x++) {
				for (int y = parentY - 1; y <= parentY + 1; y++) {
					if ((x != parentX && y != parentY) || (x == parentX && y == parentY))
						continue;

					// If not off the map (avoiding array-out-of-bounds errors)
					if (x != -1 && y != -1 && x != mine->GetHeight() && y != mine->GetWidth()) {
						
						// If not already on the closed list (items on the closed list have already been considered and can now be ignored).
						if (whichList[x][y] != inClosedList) {
							// If not a wall/obstacle square.
							if (mine->isWalkable(x, y)) {
								// If cell is not already on the open list, add it to the open list.
								if (whichList[x][y] != inOpenList) {
									numberOfOpenListItems++;					// increment number of items in the heap
									openList[numberOfOpenListItems].SetX(x);	// record the x and y coordinates of the new item
									openList[numberOfOpenListItems].SetY(y);

									// Figure out its G cost
									openList[numberOfOpenListItems].SetGcost(Gcost + 1);
									
									// Figure out its H and F costs and parent
									parent[x][y].first = parentX;							// change the cell's parent
									parent[x][y].second = parentY;
									// F cost includes H cost except when we want to use A* algorithm as Dijkstra's algorithm
									openList[numberOfOpenListItems].SetHcost( (abs(x - target.first) + abs(y - target.second)) );
									openList[numberOfOpenListItems].CalculateFcost();	// update the F cost
									
									// Move the new open list item to the proper place in the binary heap.
									BubbleItemInBinaryHeap(openList, numberOfOpenListItems);

									whichList[x][y] = inOpenList;	// Change whichList value.
								}
								// If cell is already on the open list, choose better G and F costs.
								else {
									index = GetItemIndexFromBinaryHeapByCoord(openList, numberOfOpenListItems, x, y);
									Gcost += 1;	// Figure out the G cost of this possible new path

									// If this path is shorter (G cost is lower) then change the parent cell, G cost and F cost. 		
									if (Gcost < openList[index].GetGcost()) {
										parent[x][y].first = parentX;			// change the cell's parent
										parent[x][y].second = parentY;
										openList[index].SetGcost(Gcost);	// change the G cost
										openList[index].CalculateFcost();	// update the F cost
										BubbleItemInBinaryHeap(openList, index);		// update cell's position on the open list
									}
								}	
							}
						}
					}
				}
			}
		} else {

// 4.3. If open list is empty then there is no path.	
		
			result = nonexistent;
			break;
		}  

// 5. If target cell is added to open list then path has been found.

		if (whichList[target.first][target.second] == inOpenList) {
			result = found;
			break;
		}
	}	// end while

// 6. Save the path if it exists.

	if (result == found) {

// 6.1. Working backwards from the target to the start by checking each cell's parent.

		int x = target.first, y = target.second;
		int tmp;
		while (true) {
			// Save path in reverse order
			resultPath.push_back(pair<int, int> (x, y));

			if (x == startX && y == startY) break;

			// Look up the parent of the current cell
			tmp = parent[x][y].first;
			y = parent[x][y].second;
			x = tmp;
		} 

// 6.2. Saving founded path

		for (int i = resultPath.size() - 2; i >= 0; i--) {
			path.push_back(resultPath[i]);
		}
		
	} else return -1;

// 7. Freeing used memory

	for (int i = 0; i < mine->GetHeight(); i++)
		delete [] whichList[i];
	for (int i = 0; i < mine->GetHeight(); i++)
		delete [] parent[i];
	delete [] openList;

	return 0;
}

// Description: Moves robot to the target cell
void Simulator::Step(int x, int y)
{
	int xold = mine->GetRobot().first;
	int yold = mine->GetRobot().second;
	mine->GetMap()[x][y] = 'R';
	mine->GetMap()[xold][yold] = ' ';
	mine->SetRobot(x, y);
}

// Description: Saves current mine state
void Simulator::MakeSnapshot()
{
	Field f = *mine;
	snapshot.push_back(f);
}

// Description: Restores last mine state
void Simulator::LoadSnapshot()
{
	*mine = snapshot.back();
	snapshot.pop_back();
}



// Description: Deletes the top item in binary heap and reorder the heap, with the lowest F cost item rising to the top.
void Simulator::DeleteTopItemFromBinaryHeap(OpenListItem * heap, int & heapLength)
{
	heapLength--;								// decrease number of items in heap
	heap[1] = heap[heapLength + 1];				// move last item up to slot #1
	SinkItemInBinaryHeap(heap, heapLength, 1);	// move item to the properly position
	
}

// Description: Finds the item in the binary heap by cell's coordinates and returns its index.
int Simulator::GetItemIndexFromBinaryHeapByCoord(OpenListItem * heap, int heapLength, const int & x, const int & y)
{
	int index = -1;
	for (int i = 1; i <= heapLength; i++) {
		if (heap[i].GetX() == x && heap[i].GetY() == y) {
			index = i;
			break;
		}
	}
	return index;
}

// Description: Bubbles target item to the properly position in binary heap
void Simulator::BubbleItemInBinaryHeap(OpenListItem * heap, int index)
{
	while (index != 1) {	// While item hasn't bubbled to the top
		// Swap items if child < parent.
		if (heap[index].GetFcost() <= heap[index/2].GetFcost()) {
			OpenListItem temp = heap[index/2];
			heap[index/2] = heap[index];
			heap[index] = temp;
			index = index/2;
		} else break;
	}
}

// Description: Sinks target item to the properly position in binary heap
void Simulator::SinkItemInBinaryHeap(OpenListItem * heap, int heapLength, int index)
{
	int curr, next = 1;

	// Repeat the following until the new item in slot #1 sinks to its proper spot in the heap.
	while (true) {
		curr = next;		
		if (2*curr + 1 <= heapLength) {	// if both children exist
			// Check if the F cost of the parent is greater than each child and select the lowest one.
			if (heap[curr].GetFcost() >= heap[2*curr].GetFcost()) 
				next = 2*curr;
			if (heap[next].GetFcost() >= heap[2*curr + 1].GetFcost())
				next = 2*curr+1;		
		} else {
			if (2*curr <= heapLength) {	// if only child #1 exists
				// Check if the F cost of the parent is greater than child #1	
				if (heap[curr].GetFcost() >= heap[2*curr].GetFcost()) 
					next = 2*curr;
			}
		}

		if (curr != next) {	// if parent's F > one of its children, swap them
			OpenListItem temp = heap[curr];
			heap[curr] = heap[next];
			heap[next] = temp;
		} else break;		// otherwise, exit loop
	}
}