#include "BCGA.hpp"

BCGA::EncodingSotnezovBCGA::EncodingSotnezovBCGA(int population_size, std::vector<int> groups_idx, Fitness optimize, int K, float C, int max_iter, int seed, OutputMode verbose): BCGA::SotnezovBCGA(
                                                 population_size, K, C, max_iter, seed, verbose)
{
    this->groups_idx = groups_idx;
    this->group_counters = std::vector<int>(groups_idx.size());
    this->fit_function = optimize;
}

void BCGA::EncodingSotnezovBCGA::check_compatibility()
{
    if((groups_idx.size() == 0) || (groups_idx.size() > n))
        throw std::out_of_range("Wrong number of group's indices was given");
    if(groups_idx[0] != 0)
        throw std::invalid_argument("First group index must be zero");
    int max_idx = -1;
    for(auto idx: groups_idx) {
        if(idx <= max_idx)
            throw std::invalid_argument("Some of group indices are given in a wrong order");
        else if((idx < 0) || (idx >= n))
            throw std::out_of_range("Found wrond index (idx <= 0) or (idx >= n)");
        max_idx = idx;
    }

    // add fictive idx
    groups_idx.push_back(n);

    columns_groups = std::vector<int>(n);
    for(int group = 0; group < groups_idx.size() - 1; group++)
        for(int j = groups_idx[group]; j < groups_idx[group + 1]; j++)
            columns_groups[j] = group;  
}

void BCGA::EncodingSotnezovBCGA::fill_counters(BooleanMatrix::BooleanMatrix& M, std::vector<bool>& columns)
{
    for(int i = 0; i < group_counters.size(); i++)
        group_counters[i] = 0;

    for(int group = 0; group < groups_idx.size() - 1; group++)
        for(int j = groups_idx[group]; j < groups_idx[group + 1]; j++)
            group_counters[group] += columns[j];
}

void BCGA::EncodingSotnezovBCGA::optimize_covering(BooleanMatrix::BooleanMatrix& M, std::vector<bool>& columns)
{
    std::vector<int> row_scores(m);
    std::vector<int> column_scores(n);
    for(int i = 0; i < m; i++)
        for(int j = 0; j < n; j++)
            if(M[i][j] & columns[j]) {
                row_scores[i]++;
                column_scores[j]++;
            }

    // iterate through columns (from worst to best), exclude if can
    std::vector<int> queue;

    switch(fit_function) {
    case Fitness::CovLen:
        queue = argsort(column_scores);
        break;
    case Fitness::MaxBinsNum:
        queue = special_conditional_argsort(column_scores, columns_groups, group_counters);
        //queue = argsort(column_scores);
        break;
    default:
        throw std::invalid_argument("Unknown fitness funtion in optimization call");
    }

    for(int i = 0; i < n; i++) {
        if(group_counters[columns_groups[queue[i]]] < 1)
            throw std::out_of_range("WTF");
        if(group_counters[columns_groups[queue[i]]] == 1)
            continue;

        //zero rows covered or not in covering
        if(!column_scores[queue[i]]) {
            if(columns[queue[i]]) {
                columns[queue[i]] = false;
                group_counters[columns_groups[queue[i]]]--;
            }
            continue;
        }

        bool flag = true;
        for(int j = 0; j < m; j++)
            if(M[j][queue[i]] && (row_scores[j] < GlobalSettings::SotnezovThreshold)) {
                flag = false;
                break;
            }

        if(!flag)
            continue;

        columns[queue[i]] = false;
        group_counters[columns_groups[queue[i]]]--;
        for(int j = 0; j < m; j++) {
            // Decrease scores
            if(M[j][queue[i]])
                row_scores[j]--;
        }
    }
}

void BCGA::EncodingSotnezovBCGA::restore_solution(BooleanMatrix::BooleanMatrix& M, std::vector<bool>& columns)
{
    std::vector<bool> covered_rows = get_covered_rows(M, columns);

    fill_counters(M, columns);
    for(int group = 0; group < groups_idx.size() - 1; group++) {
        if(group_counters[group] > 0)
            continue;

        // add random column from group
        std::uniform_int_distribution<> columns_d(groups_idx[group], groups_idx[group + 1] - 1);
        int random_column = columns_d(rng);
        columns[random_column] = true;
        for(int i = 0; i < m; i++)
            covered_rows[i] = (covered_rows[i] || M[i][random_column]);
    }

    for(int i = 0; i < covered_rows.size(); i++) {
        if(covered_rows[i])
            continue;
        add_maxscore_column(M, covered_rows, columns, i, 0, n);
    }

    fill_counters(M, columns);
}

double BCGA::EncodingSotnezovBCGA::covlen_fitness(BooleanMatrix::BooleanMatrix& M, BCGA::BinaryIndividual& individual)
{
    int ones_counter = 0;
    for(int i = 0; i < individual.size(); i++)
        if(individual.genotype[i])
            ones_counter++;
    if(ones_counter < groups_idx.size() - 1)
        throw "A bug was detected";
    return ones_counter;
}

double BCGA::EncodingSotnezovBCGA::maxbinsnum_fitness(BooleanMatrix::BooleanMatrix& M, BCGA::BinaryIndividual& individual)
{
    fill_counters(M, individual.genotype);
    int max_bin_num = 0;
    for(auto bin_num: group_counters)
        if(bin_num > max_bin_num)
            max_bin_num = bin_num;
    return max_bin_num + 1;
}

double BCGA::EncodingSotnezovBCGA::fitness(BooleanMatrix::BooleanMatrix& M, BCGA::BinaryIndividual& individual)
{
    switch(fit_function) {
    case Fitness::CovLen:
        return covlen_fitness(M, individual);
    case Fitness::MaxBinsNum:
        return maxbinsnum_fitness(M, individual);
    default:
        throw std::invalid_argument("Unknown fitness funtion in fitness call");
    }

    throw std::invalid_argument("Unknown fitness funtion in fitness call");
    return 0;
}
