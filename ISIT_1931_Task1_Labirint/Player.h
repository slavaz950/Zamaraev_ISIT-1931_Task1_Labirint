/*
 * Player.h
 *
 *  Created on: 28 авг. 2022 г.
 *      Author: Zver
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include <memory>
#include "Map.h"

class Player final
{
	private:
		double _x {}, _y {}, _dir {};
		std::shared_ptr<Map> _map;

		void walk(double angle, double dist);

	public:

		void spawn(std::shared_ptr<Map> &m);



		double x() 		const { return _x; }
		double y() 		const { return _y; }
		double dir() 	const { return _dir; }

		void walk_forward(double dist);
		void walk_back(double dist);
		void shift_left(double dist);
		void shift_right(double dist);
		void turn_left(double angle);
		void turn_right(double angle);

};

#endif /* PLAYER_H_ */

