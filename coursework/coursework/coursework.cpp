// coursework.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>
#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <chrono>

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
		cout << "Number of cols: " << cols << std::endl;
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
	cout << "Rows number: " << row_counter << std::endl;
	sqlite3_finalize(statement);
	return entries;
}

double avg(vector<entry> v)
{
	double sum = 0;
	double n = 0;
	for (vector<entry>::iterator it = v.begin(); it != v.end(); ++it)
	{
		sum += it->rating;
		n++;
	}
	return sum / n;
	//return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
	//return accumulate(v.begin(), v.end(), 0.0 / v.size());
}

bool compareItems(const entry &a, const entry &b)
{
	return a.item < b.item || a.item == b.item && a.user < b.user;
}

double pearson_similarity(int u1, int u2, sqlite3 *db)
{
	if (u1 == u2)
	{
		return 1.0;
	}
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	string select1 = "SELECT * from usersratings WHERE usersratings.user in (" + std::to_string(u1) + "," + to_string(u2) + ")";
	//string select2 = "SELECT * from usersratings WHERE usersratings.user = " + std::to_string(u2);
	vector<entry> users = query(select1.c_str(), db);

	high_resolution_clock::time_point t2 = high_resolution_clock::now();
	auto duration = duration_cast<milliseconds>(t2 - t1).count();
	cout << "database query duration in milliseconds: " << duration << std::endl;

	vector<entry> user1;
	vector<entry> user2;

	high_resolution_clock::time_point t5 = high_resolution_clock::now();

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
	high_resolution_clock::time_point t6 = high_resolution_clock::now();

	auto duration2 = duration_cast<milliseconds>(t6 - t5).count();
	cout << "separate database output duration in milliseconds: " << duration2 << std::endl;

	vector<entry> temp1 = user1;
	vector<entry> temp2 = user2;

	high_resolution_clock::time_point t3 = high_resolution_clock::now();
	double avg1 = avg(user1);
	double avg2 = avg(user2);
	high_resolution_clock::time_point t4 = high_resolution_clock::now();
	auto duration1 = duration_cast<microseconds>(t4 - t3).count();
	cout << "avg method duration in microseconds: " << duration1 << std::endl;
	//cout << "AVG 1: " << avg1 << std::endl;
	//cout << "AVG 2: " << avg2 << std::endl;

	high_resolution_clock::time_point t10 = high_resolution_clock::now();
	user2.insert(user2.end(), user1.begin(), user1.end());
	std::sort(user2.begin(), user2.end(), compareItems);
	//high_resolution_clock::time_point t11 = high_resolution_clock::now();
	int matches = 0;
	double newnum = 0;
	double newden1 = 0;
	double newden2 = 0;
	
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
	
	high_resolution_clock::time_point t11 = high_resolution_clock::now();
	auto duration5 = duration_cast<milliseconds>(t11 - t10).count();
	cout << "NEW pearson coeff duration in milliseconds: " << duration5 << std::endl;
	cout << "number of matches: " << matches << std::endl;
	return newnum / (sqrt(newden1) * sqrt(newden2));
	//cout << "NEW coeff is: " << newnum / (sqrt(newden1) * sqrt(newden2)) << std::endl;

	/*high_resolution_clock::time_point t7 = high_resolution_clock::now();
	double num = 0;
	double den1 = 0;
	double den2 = 0;
	int matches2 = 0;
	for (int i=0; i<temp1.size(); i++)
	{
		for (int j = 0; j < temp2.size(); j++)
		{
			if (temp1[i].item == temp2[j].item)
			{
				matches2++;
				double diff1 = (temp1[i].rating - avg1);
				double diff2 = (temp2[j].rating - avg2);
				num +=  diff1 * diff2;
				den1 += pow(diff1, 2);
				den2 += pow(diff2, 2);
			}
		}
	}

	high_resolution_clock::time_point t8 = high_resolution_clock::now();
	auto duration3 = duration_cast<milliseconds>(t8 - t7).count();
	cout << "OLD pearson coeff duration in milliseconds: " << duration3 << std::endl;
	cout << "OLD matches: " << matches2 << std::endl;
	return num / (sqrt(den1) * sqrt(den2));*/
}

int callback(void *data, int argc, char **argv, char **azColName) {
	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
}

int main(int argc, const char * argv[]) {

	int rc;
	sqlite3 *db;
	rc = sqlite3_open("C:/sqlite/test", &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		return(0);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	// increase performance
	//rc = sqlite3_exec(db, "BEGIN TRANSACTION", 0, 0, 0);
	char *zErrMsg = 0;

	/*int user = 2;
	string select = "SELECT * from usersratings WHERE usersratings.user = " + std::to_string(user);
	const char *sql = select.c_str();
	//char *sql = concat("SELECT * from usersratings WHERE usersratings.user = ", user);
	vector<entry> u1 = query(sql, db);
	for (int i = 0; i < u1.size(); i++)
	{
		cout << u1.at(i).user << " " << u1.at(i).item << " " << u1.at(i).rating << std::endl;
	}*/
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	double sim = pearson_similarity(15, 15, db);
	high_resolution_clock::time_point t2 = high_resolution_clock::now();

	auto duration = duration_cast<milliseconds>(t2 - t1).count();

	cout << "total duration in milliseconds: " << duration << std::endl;
	cout << "Similarity is: " << sim << std::endl;
	if (isnan(sim))
	{
		cout << "Similiraty is Not A Number" << std::endl;
	}

	// database index 

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

	// in memory 
	// 

	sqlite3_close(db);
    return 0;
}

