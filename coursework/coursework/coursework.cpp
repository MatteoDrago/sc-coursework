// coursework.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>
#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <omp.h>
#include <string>

using namespace std;
using namespace std::chrono;

struct entry {
	int user;
	int item;
	int rating;
	int avg = 0;
};

vector<entry> query(const char* query, sqlite3 *db)
{
	sqlite3_stmt *statement;
	vector<entry> entries;
	int row_counter = 0;

	if (sqlite3_prepare_v2(db, query, -1, &statement, 0) == SQLITE_OK)
	{
		int cols = sqlite3_column_count(statement);
		int result = 0;
		while (true)
		{
			result = sqlite3_step(statement);
			if (result == SQLITE_ROW)
			{
				row_counter++;
				entry row_entry;
				for (int i = 0; i < cols; i++)
				{
					switch (i)
					{
						case(0):
							row_entry.user = sqlite3_column_int(statement, i);
							break;
						case(1):
							row_entry.item = sqlite3_column_int(statement, i);
							break;
						case(2):
							row_entry.rating = sqlite3_column_int(statement, i);
							break;	
					}
				}
				entries.push_back(row_entry);
			}
			else
			{
				break;
			}
		}
	}
	sqlite3_finalize(statement);
	return entries;
}

double avg(vector<entry> v)
{
	int sum = 0;
	int n = 0;
	for (vector<entry>::iterator it = v.begin(); it != v.end(); ++it)
	{
		sum += it->rating;
		n++;
	}
	return sum / double(n);
}

bool compareItems(const entry &a, const entry &b)
{
	return a.item < b.item || a.item == b.item && a.user < b.user;
}

bool compareRatings(const entry &a, const entry &b)
{
	return a.rating < b.rating || a.rating == b.rating && a.user < b.user;
}

double hamming(int u1, int u2, sqlite3 *db)
{
	if (u1 == u2)
	{
		return 1.0;
	}
	string select1 = "SELECT * from usersratings WHERE usersratings.user = " + std::to_string(u1);
	string select2 = "SELECT * from usersratings WHERE usersratings.user = " + std::to_string(u2);
	vector<entry> user1 = query(select1.c_str(), db);
	vector<entry> user2 = query(select2.c_str(), db);
	//users.insert(users.end(), user2.begin(), user2.end());
	//std::sort(users.begin(), users.end(), compareRatings);
	vector<entry> entries;
	uint32_t minsize = min(user1.size(), user2.size());
	uint32_t maxsize = max(user1.size(), user2.size());
	for (int i = 0; i < user1.size(); i++)
	{
		cout << user1[i].user << " " << user1[i].rating << std::endl;
	}
	for (int i = 0; i < user2.size(); i++)
	{
		cout << user2[i].user << " " << user2[i].rating << std::endl;
	}
	for (uint32_t i = 0; i < minsize; i++)
	{
		entries.push_back(user1[i]);
		entries.push_back(user2[i]);
	}
	for (uint32_t i = minsize; i < maxsize; i++)
	{
		if (user1.size() > user2.size())
		{
			entries.push_back(user1[i]);
		}
		else {
			entries.push_back(user2[i]);
		}
	}

	int dist = 0;
	int n = 0;
	#pragma omp parallel for reduction(+:dist), reduction(:+n);
	for (int i = 0; i < entries.size()-1; i+=2)
	{
		if (entries[i].user != entries[i + 1].user)
		{
			dist += (entries[i].rating != entries[i + 1].rating);
			n++;
		}
	}
	cout << "minsize " << minsize << std::endl;
	cout << "n " << n << std::endl;
	return dist / double(n);
}

double cosine_similarity(int i1, int i2, sqlite3 *db)
{
	string select1 = "SELECT * from usersratings WHERE usersratings.item in (" + std::to_string(i1) + "," + to_string(i2) + ")";
	vector<entry> items = query(select1.c_str(), db);
	vector<entry> item1;
	vector<entry> item2;
	for (int i = 0; i < items.size(); i++)
	{
		if (items[i].item == i1)
		{
			item1.push_back(items[i]);
		}
		else {
			item2.push_back(items[i]);
		}
	}
	return 1.0;
}

int runTransaction(sqlite3 *db_avg, vector<const char*> insert_query)
{
	if (!db_avg)
		return 0;
	char *zErrMsg = 0;
	// start sqlite transaction
	sqlite3_exec(db_avg, "BEGIN TRANSACTION", 0, 0, 0);
	cout << "number of query " << insert_query.size() << std::endl;
	for (int i = 0; i < insert_query.size(); i++)
	{
		const char *query = insert_query[i];
		int rc = sqlite3_exec(db_avg, query, NULL, 0, &zErrMsg);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
			// rollback all update/insert to sqlite 
			sqlite3_exec(db_avg, "ROLLBACK", 0, 0, 0);
			return 0; // if an error occur
		}
	}
	// commit all to sqlite database
	sqlite3_exec(db_avg, "COMMIT", 0, 0, 0);
	return 1;
}

void calculate_avg_ratings(sqlite3 *db, sqlite3 *db_avgs, long int tot_users)
{
	long int commit_value = 10;
	long int n = 1;
	int trans_number = 1;
	vector<const char*> insert_query;
	cout << "Starting creating avg database..." << std::endl;
	
	while (n <= tot_users)
	{
		string string_db = "SELECT * from usersratings WHERE usersratings.user = " + to_string(n);
		vector<entry> user = query(string_db.c_str(), db);
		//string to_insert = "insert into averages (user, avg) values (" + to_string(n) +", "+ to_string(avg(user)) + ")";
		string to_insert = "INSERT INTO averages(user, avg)";
		to_insert.append(" VALUES(");
		to_insert.append(to_string(n).c_str());
		to_insert.append(", ");
		to_insert.append(to_string(avg(user)).c_str());
		to_insert.append(");");
		cout << to_insert << std::endl;
		const char *query = to_insert.c_str();
		insert_query.push_back(to_insert.c_str());
		if ((n % commit_value) == 0 || n == tot_users)
		{
			cout << "value of n is: " << n << std::endl;
			if (runTransaction(db_avgs, insert_query) == 0) {
				break;
			}
			insert_query.clear();
			cout << "Done " << trans_number << ", insert avgs of " << commit_value << " users" << std::endl;
			trans_number++;
		}
		n++;
	}
	cout << "Finished successfully avgs db creation" << std::endl;
}

double pearson_similarity(int u1, int u2, sqlite3 *db)
{
	if (u1 == u2)
	{
		return 1.0;
	}
	string select1 = "SELECT * from usersratings WHERE usersratings.user in (" + std::to_string(u1) + "," + to_string(u2) + ")";
	//string select2 = "SELECT * from usersratings WHERE usersratings.user = " + std::to_string(u2);
	vector<entry> users = query(select1.c_str(), db);
	vector<entry> user1;
	vector<entry> user2;

	for (int i = 0; i < users.size(); i++)
	{
		if (users[i].user == u1)
		{	
			user1.push_back(users[i]);
		}
		else {
			user2.push_back(users[i]);
		}
	}
	double avg1 = avg(user1);
	double avg2 = avg(user2);
	user2.insert(user2.end(), user1.begin(), user1.end());
	std::sort(user2.begin(), user2.end(), compareItems);

	int matches = 0;
	double newnum = 0;
	double newden1 = 0;
	double newden2 = 0;
	#pragma omp parallel for reduction(+:newnum), reduction(+:newden1), reduction(+:newden2);

	for (int i = 0; i < user2.size() - 1; i++)
	{
		if (user2[i].item == user2[i + 1].item)
		{
			double avg_int1;
			double avg_int2;
			if (user2[i].user == u1)
			{
				avg_int1 = avg1;
			}
			else {
				avg_int1 = avg2;
			}
			if (user2[i+1].user == u1)
			{
				avg_int2 = avg1;
			}
			else {
				avg_int2 = avg2;
			}
			double diff1 = (user2[i].rating - avg_int1);
			double diff2 = (user2[i+1].rating - avg_int2);
			newnum += diff1 * diff2;
			newden1 += pow(diff1, 2);
			newden2 += pow(diff2, 2);
			matches++;
		}
	}
	return newnum / (sqrt(newden1) * sqrt(newden2));
}


int main(int argc, const char * argv[]) {

	int rc;
	sqlite3 *db;
	rc = sqlite3_open("C:/sqlite/test", &db);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	int avgs;
	sqlite3 *db_avgs;
	avgs = sqlite3_open("C:/sqlite/medie", &db_avgs);
	if (avgs != SQLITE_OK) {
		fprintf(stderr, "Can't open averages database: %s\n", sqlite3_errmsg(db));
		return 1;
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}

	// calculate mean of ratings for each users an store it to a db_avgs:
	// 1. retrieve total number of users
	char *max_users_query = "SELECT * from usersratings WHERE user=(SELECT max(user) FROM usersratings)";
	vector<entry> max = query(max_users_query, db);

	cout << "total number of users is: " << max[0].user << std::endl;
	long int tot_users = max[0].user;

	bool avg = true;
	if (avg)
	{
		calculate_avg_ratings(db, db_avgs, tot_users);
	}

	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	double sim = pearson_similarity(15, 6, db);
	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(t2 - t1).count();
	cout << "total duration in milliseconds: " << duration << std::endl;
	cout << "Similarity is: " << sim << std::endl;
	if (isnan(sim))
	{
		cout << "Similiraty is Not A Number" << std::endl;
	} 

	high_resolution_clock::time_point t3 = high_resolution_clock::now();
	double ham = hamming(6, 1350, db);
	high_resolution_clock::time_point t4 = high_resolution_clock::now();
	auto duration_ham = duration_cast<milliseconds>(t4 - t3).count();
	cout << "total duration of hamming in milliseconds: " << duration_ham << std::endl;
	cout << "Hamming distance is: " << ham << std::endl;

	/*std::vector<entry> u;
	//Execute SQL statement
	//rc = sqlite3_exec(db, sql, callback, &u, &zErrMsg);

	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else {
		fprintf(stdout, "Operation done successfully\n");
	}
	*/

	sqlite3_close(db);
    return 0;
}

