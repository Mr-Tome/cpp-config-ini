#include <iostream>

#include <vector>
#include <string>
#include <stdlib.h>

int main(){
	std::string finishedAnswer;
	while(true)
	{
		std::cout << "CPP Template is great (yes/no)?\n";
		std::cin >> finishedAnswer;
		if(finishedAnswer=="yes" ||
		finishedAnswer=="y" ||
		finishedAnswer=="si" ||
		finishedAnswer=="oui")
		{
			return 0;
		}
	}
	return 0;
}
