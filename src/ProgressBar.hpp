/*
	This file is part of cgCompress.

	cgCompress is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cgCompress is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cgCompress.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PROGRESS_BAR_HPP
#define PROGRESS_BAR_HPP

#include <string>
#include <iostream>

/** Creates a progress bar on stdout with a title. Scope is used to stop the
 *  progress bar, do not output anything to stdout until the destructor is
 *  called */
class ProgressBar{
	private:
		int amount;
		int size;
		int count{ 0 };
		int written{ 0 };
		
	public:
		/** Create the progress bar
		 *  
		 *  \param [in] msg A title to be displayed together with the progress
		 *  \param [in] amount The amount of task to be done
		 *  \param [in] size The width of the progress bar
		 */
		ProgressBar( std::string msg, int amount, int size=60 ) : amount(amount), size(size){
			//Print slightly fancy header with centered text
			msg += " (" + std::to_string( amount ) + ")";
			int left = size - msg.size();
			
			for( int i=0; i<size; i++ )
				std::cout << "_";
			
			std::cout << std::endl << "|";
			for( int i=1; i<left/2; i++ )
				std::cout << " ";
			std::cout << msg;
			for( int i=1; i<left-left/2; i++ )
				std::cout << " ";
			std::cout << "|" << std::endl;
		}
		/** Stops and closes the progress bar */
		~ProgressBar(){ std::cout << std::endl; }
		
		/** Advance the progress
		 * \param [in] progress How much progress that have been made
		 */
		void update( int progress=1 ){
			for( count += progress; written < count*size/amount; written++ )
				std::cout << "X";
		}
};

#endif

