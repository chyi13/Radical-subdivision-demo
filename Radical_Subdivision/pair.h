#ifndef _PAIR_H_
#define _PAIR_H_

//#include <iostream>
//
//using namespace std;
//using std::ostream;

class Pair{

	/*friend ostream &operator<<( ostream& ,const Pair&);*/
	
public:
	int a,b;

	
	Pair(int i,int j){
		a = i;
		b = j;
	};
	
	//~Pair(void){};

	Pair(const Pair &init){
		a = init.a;
		b = init.b;
	}

	//int getA(void){ return a;}
	//int getB(void){ return b;}
	
	bool operator== (const Pair &p) const{
		if(a == p.a && b == p.b)
			return true;
		else
			return false;
	}

	bool operator< (const Pair &p) const{
		if(a < p.a)		
			return true;			
		else if(a >p.a)			
			return false;
		else{
			if(b < p.b)
				return true;
			else
				return false;
		}

	}

	bool operator> (const Pair &p) const{
		if(a > p.a)
			return true;
		else if( b > p.b)
			return false;
		else{
			if(b > p.b)
				return true;
			else
				return false;
		}

	}
};

//ostream &operator<< (ostream& os, const Pair &p){
//	os<< "(" << p.a << "," << p.b << ")";
//	return os;
//}
#endif

