#include <iostream>
#include <vector>
#include <random>
#include <numeric>
#include <algorithm>

namespace Genetic {
    class BooleanMatrix;
    class Individual;
    class GeneticAlgorithm;
    std::ostream& operator<<(std::ostream& os, const Individual& I);
};

class Genetic::BooleanMatrix {
    int m;
    int n;
public:
    bool** M;
    BooleanMatrix(int m, int n);
    BooleanMatrix(const BooleanMatrix& B);
    int get_m();
    int get_n();
    bool* operator[](int);
    ~BooleanMatrix();

};

class Genetic::Individual {
    std::vector<bool> chromosome;

public:
    Individual(int chromosome_size);
    Individual(std::vector<bool> chromosome);
    bool is_coverage(Genetic::BooleanMatrix& M);
    double fitness(Genetic::BooleanMatrix& M);

    friend Genetic::GeneticAlgorithm;
    friend std::ostream& Genetic::operator<<(std::ostream& os, const Genetic::Individual& I);
};

class Genetic::GeneticAlgorithm {
    std::vector<Individual> population;
    std::vector<int> scores;

    std::mt19937 rng;

    int max_iter;
    int population_size;
    int extended_population_size;
    double mutation_proba;

    template <typename T>
    std::vector<int> argsort(const std::vector<T> &v) {
        //Argsort for any type of elem-comparable vectors
        std::vector<int> idx(v.size());
        std::iota(idx.begin(), idx.end(), 0);

        std::stable_sort(idx.begin(), idx.end(),
            [&v](int i1, int i2) {return v[i1] < v[i2];});

        return idx;
    }

public:
    GeneticAlgorithm(int population_size, int extended_population_size, int chromosome_len,
                     double mutation_proba, int max_iter);
    Individual one_point_crossover(Individual s1, Individual s2);
    void fit(BooleanMatrix& M, int verbose, bool finishing_message);
    std::vector<bool> get_best_chromosome();
    void print_solution(BooleanMatrix& M);
};
