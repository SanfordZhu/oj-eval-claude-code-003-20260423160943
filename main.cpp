#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <cstring>

using namespace std;

struct Submission {
    string team;
    string problem;
    string status;
    int time;

    Submission(string t_name, string p, string s, int t)
        : team(t_name), problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int solved_time;
    int wrong_attempts;
    int frozen_submissions;
    bool is_solved;
    bool is_frozen;

    ProblemStatus() : solved_time(0), wrong_attempts(0), frozen_submissions(0),
                      is_solved(false), is_frozen(false) {}
};

struct Team {
    string name;
    vector<ProblemStatus> problems;
    int total_solved;
    int total_penalty;
    vector<int> solve_times;
    int rank;

    Team(string n, int problem_count) : name(n), problems(problem_count),
                                       total_solved(0), total_penalty(0), rank(0) {}
};

struct CompareTeams {
    bool operator()(const Team* a, const Team* b) const {
        if (a->total_solved != b->total_solved) {
            return a->total_solved > b->total_solved;
        }

        if (a->total_penalty != b->total_penalty) {
            return a->total_penalty < b->total_penalty;
        }

        int n = min(a->solve_times.size(), b->solve_times.size());
        for (int i = 0; i < n; i++) {
            if (a->solve_times[i] != b->solve_times[i]) {
                return a->solve_times[i] < b->solve_times[i];
            }
        }

        return a->name < b->name;
    }
};

class ICPMSystem {
private:
    map<string, Team*> teams;
    vector<Team*> team_list;
    vector<Submission> submissions;
    bool competition_started;
    bool frozen;
    int duration_time;
    int problem_count;
    int flush_count;

public:
    ICPMSystem() : competition_started(false), frozen(false),
                   duration_time(0), problem_count(0), flush_count(0) {}

    ~ICPMSystem() {
        for (auto& team : team_list) {
            delete team;
        }
    }

    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }

        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }

        Team* team = new Team(team_name, 26);
        teams[team_name] = team;
        team_list.push_back(team);
        cout << "[Info]Add successfully.\n";
    }

    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }

        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        cout << "[Info]Competition starts.\n";
    }

    void submit(const string& problem, const string& team_name,
                const string& status, int time) {
        if (!competition_started) return;

        submissions.emplace_back(team_name, problem, status, time);

        Team* team = teams[team_name];
        int problem_idx = problem[0] - 'A';

        if (frozen && !team->problems[problem_idx].is_solved) {
            team->problems[problem_idx].is_frozen = true;
        }

        if (status == "Accepted" && !team->problems[problem_idx].is_solved) {
            team->problems[problem_idx].is_solved = true;
            team->problems[problem_idx].solved_time = time;
        } else if (!team->problems[problem_idx].is_solved) {
            if (team->problems[problem_idx].is_frozen) {
                team->problems[problem_idx].frozen_submissions++;
            } else {
                team->problems[problem_idx].wrong_attempts++;
            }
        }

        if (!frozen || !team->problems[problem_idx].is_frozen) {
            updateTeamScore(team);
        }
    }

    void updateTeamScore(Team* team) {
        team->total_solved = 0;
        team->total_penalty = 0;
        team->solve_times.clear();

        for (int i = 0; i < problem_count; i++) {
            if (team->problems[i].is_solved && !team->problems[i].is_frozen) {
                team->total_solved++;
                int penalty = 20 * team->problems[i].wrong_attempts +
                             team->problems[i].solved_time;
                team->total_penalty += penalty;
                team->solve_times.push_back(team->problems[i].solved_time);
            }
        }

        sort(team->solve_times.begin(), team->solve_times.end(), greater<int>());
    }

    void flush() {
        if (!competition_started) return;

        updateRankings();
        cout << "[Info]Flush scoreboard.\n";
        flush_count++;
    }

    void freeze() {
        if (!competition_started) return;

        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }

        frozen = true;
        flush();
        cout << "[Info]Freeze scoreboard.\n";
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        flush();
        vector<Team*> before_ranking = getCurrentRanking();
        printScoreboard(before_ranking);

        vector<pair<Team*, int>> teams_with_frozen;
        for (Team* team : team_list) {
            for (int i = 0; i < problem_count; i++) {
                if (team->problems[i].is_frozen) {
                    teams_with_frozen.push_back({team, i});
                }
            }
        }

        while (!teams_with_frozen.empty()) {
            Team* lowest_team = nullptr;
            int lowest_problem = -1;
            int lowest_rank = -1;

            for (auto& [team, prob] : teams_with_frozen) {
                int rank = 0;
                for (int i = 0; i < before_ranking.size(); i++) {
                    if (before_ranking[i] == team) {
                        rank = i;
                        break;
                    }
                }

                if (lowest_rank == -1 || rank > lowest_rank ||
                    (rank == lowest_rank && prob < lowest_problem)) {
                    lowest_rank = rank;
                    lowest_team = team;
                    lowest_problem = prob;
                }
            }

            if (!lowest_team) break;

            lowest_team->problems[lowest_problem].is_frozen = false;

            // Check if this problem was actually solved (after freeze)
            if (lowest_team->problems[lowest_problem].is_solved) {
                updateTeamScore(lowest_team);

                vector<Team*> after_ranking = getCurrentRanking();

                int old_pos = 0, new_pos = 0;
                for (int i = 0; i < before_ranking.size(); i++) {
                    if (before_ranking[i] == lowest_team) old_pos = i;
                    if (after_ranking[i] == lowest_team) new_pos = i;
                }

                if (old_pos != new_pos) {
                    cout << lowest_team->name << " "
                         << before_ranking[new_pos]->name << " "
                         << lowest_team->total_solved << " "
                         << lowest_team->total_penalty << "\n";
                }

                before_ranking = after_ranking;
            }

            updateTeamScore(lowest_team);
            vector<Team*> after_ranking = getCurrentRanking();

            int old_pos = 0, new_pos = 0;
            for (int i = 0; i < before_ranking.size(); i++) {
                if (before_ranking[i] == lowest_team) old_pos = i;
                if (after_ranking[i] == lowest_team) new_pos = i;
            }

            if (old_pos != new_pos) {
                cout << lowest_team->name << " "
                     << before_ranking[new_pos]->name << " "
                     << lowest_team->total_solved << " "
                     << lowest_team->total_penalty << "\n";
            }

            before_ranking = after_ranking;

            teams_with_frozen.clear();
            for (Team* team : team_list) {
                for (int i = 0; i < problem_count; i++) {
                    if (team->problems[i].is_frozen) {
                        teams_with_frozen.push_back({team, i});
                    }
                }
            }
        }

        frozen = false;
        printScoreboard(before_ranking);
    }

    void queryRanking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query ranking.\n";

        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }

        updateRankings();
        Team* team = teams[team_name];
        cout << team->name << " NOW AT RANKING " << team->rank << "\n";
    }

    void querySubmission(const string& team_name, const string& problem,
                        const string& status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }

        cout << "[Info]Complete query submission.\n";

        Submission* last_match = nullptr;

        // Debug: print all submissions
        // for (auto& sub : submissions) {
        //     cerr << "DEBUG: Submission: " << sub.team << " " << sub.problem << " " << sub.status << " " << sub.time << endl;
        // }

        // Debug query parameters
        // cerr << "DEBUG: Query params: team=" << team_name << " problem=" << problem << " status=" << status << endl;

        for (auto& sub : submissions) {
            bool match_problem = (problem == "ALL" || sub.problem == problem);
            bool match_status = (status == "ALL" || sub.status == status);
            bool match_team = (sub.team == team_name);

            if (match_problem && match_status && match_team) {
                last_match = &sub;
            }
        }

        if (!last_match) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << team_name << " " << last_match->problem << " "
                 << last_match->status << " " << last_match->time << "\n";
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
    }

private:
    vector<Team*> getCurrentRanking() {
        vector<Team*> ranking = team_list;
        sort(ranking.begin(), ranking.end(), CompareTeams());

        for (int i = 0; i < ranking.size(); i++) {
            ranking[i]->rank = i + 1;
        }

        return ranking;
    }

    void updateRankings() {
        sort(team_list.begin(), team_list.end(), CompareTeams());

        for (int i = 0; i < team_list.size(); i++) {
            team_list[i]->rank = i + 1;
        }
    }

    void printScoreboard(const vector<Team*>& ranking) {
        for (Team* team : ranking) {
            cout << team->name << " " << team->rank << " "
                 << team->total_solved << " " << team->total_penalty;

            for (int i = 0; i < problem_count; i++) {
                cout << " ";

                if (team->problems[i].is_frozen) {
                    if (team->problems[i].wrong_attempts == 0) {
                        cout << "0/" << team->problems[i].frozen_submissions;
                    } else {
                        cout << "-" << team->problems[i].wrong_attempts
                             << "/" << team->problems[i].frozen_submissions;
                    }
                } else if (team->problems[i].is_solved) {
                    if (team->problems[i].wrong_attempts == 0) {
                        cout << "+";
                    } else {
                        cout << "+" << team->problems[i].wrong_attempts;
                    }
                } else {
                    if (team->problems[i].wrong_attempts == 0) {
                        cout << ".";
                    } else {
                        cout << "-" << team->problems[i].wrong_attempts;
                    }
                }
            }

            cout << "\n";
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPMSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.addTeam(team_name);
        } else if (cmd == "START") {
            string duration_str, problem_str;
            int duration, problems;
            iss >> duration_str >> duration >> problem_str >> problems;
            system.startCompetition(duration, problems);
        } else if (cmd == "SUBMIT") {
            string problem, by, team_name, with, status, at, time_str;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time_str;
            time = stoi(time_str);
            system.submit(problem, team_name, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, problem, and_str, status;
            iss >> team_name >> where >> problem >> and_str >> status;

            // Extract problem value (remove PROBLEM= prefix)
            if (problem.find("PROBLEM=") == 0) {
                problem = problem.substr(8);
            }

            // Extract status value (remove STATUS= prefix)
            if (status.find("STATUS=") == 0) {
                status = status.substr(7);
            }

            system.querySubmission(team_name, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }

    return 0;
}