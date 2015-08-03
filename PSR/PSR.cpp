#include <iostream>
#include <random>
#include <cctype>
#include <exception>
#include <stdexcept>
#include <string>
#include <sstream>

using namespace std;

class HandType
{
public:
	enum EnumType { ROCK, PAPER, SCISSOR };

private:
	EnumType type;

public:
	explicit HandType(EnumType mytype) : type(mytype) {}
	explicit HandType(char mytype)
	{
		mytype = tolower(mytype);
		switch (mytype)
		{
		case 'r':
			type = ROCK;
			break;
		case 'p':
			type = PAPER;
			break;
		case 's':
			type = SCISSOR;
			break;
		default:
			throw std::invalid_argument(string("Unknown hand type: ") + mytype);
		}
	}

	bool operator< (const HandType& rhs) const
	{
		if (type == PAPER && rhs.type == SCISSOR)
			return true;
		if (type == ROCK && rhs.type == PAPER)
			return true;
		if (type == SCISSOR && rhs.type == ROCK)
			return true;

		return false;
	}
	bool operator> (const HandType& rhs) const
	{
		return rhs.operator<(*this);
	}

	friend ostream& operator<<(ostream& os, const HandType& handType);
};

ostream& operator<<(ostream& os, const HandType& handType)
{
	char charType = ' ';
	switch (handType.type)
	{
	case HandType::ROCK:
		charType = 'r';
		break;
	case HandType::PAPER:
		charType = 'p';
		break;
	case HandType::SCISSOR:
		charType = 's';
		break;
	}
	os << charType;
	return os;
}

class RandomHand
{
private:
	std::random_device rd;
	std::default_random_engine e1;
	std::uniform_int_distribution<unsigned int> uniform_dist;
public:
	RandomHand() : uniform_dist(0, 2)
	{
		e1.seed(rd());
	}
	inline HandType::EnumType show()
	{
		return static_cast<HandType::EnumType>(uniform_dist(e1));
	}

};

class Match
{
private:
	unsigned int roundsLeft;
	RandomHand computerHand;
	unsigned int playerWon;
	unsigned int computerWon;
public:
	explicit Match(unsigned int rounds) : roundsLeft(rounds), playerWon(0), computerWon(0) {}
	void Play()
	{
		while (roundsLeft > 0)
		{
			PlayRound();
		}
		cout << "End of the game! " << endl;

		if (computerWon > playerWon) cout << "Computer wins the game.";
		else if (computerWon < playerWon) cout << "Player wins the game.";
		else cout << "Draw - no one wins the game.";

	}

	void PlayRound()
	{
		char playerHandChar;
		cout << "Choose your hand: ";
		cin >> playerHandChar;

		try
		{
			HandType playerHand(playerHandChar);

			HandType::EnumType computerHandType = computerHand.show();
			HandType computerHand(computerHandType);
			cout << "Computer choose: ";
			cout << computerHand << endl;
			EndRound(playerHand, computerHand);
		}
		catch (invalid_argument&)
		{
			Help();
			return;
		}
	}

	void EndRound(HandType player, HandType computer)
	{
		if (player > computer)
		{
			playerWon++;
			cout << "Player wins.";
		}
		else if (player < computer)
		{
			computerWon++;
			cout << "Computer wins.";
		}
		else
		{
			cout << "Draw.";
		}
		cout << endl;
		roundsLeft--;
	}

	void Help()
	{
		cout << "For Paper type 'p', Scissor type 's', Rock type 'r'. " << endl;
	}
};

int main()
{
	unsigned int rounds;
	do
	{
		cout << "How many rounds do you want to play [1-1000]?" << endl;
		string tmp;
		std::getline(std::cin, tmp);
		rounds = atoi(tmp.c_str());
	} while (rounds == 0 || rounds > 1000);

	Match match(rounds);
	match.Help();
	match.Play();

	return 0;
}