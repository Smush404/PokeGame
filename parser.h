#ifndef PARSER_H
# define PARSER_H

#include <string>

using namespace std;

class pokemon{
    public: 
        int ID, 
            species_id, 
            height, 
            weight,
            base_experience, 
            order,
            is_default;

        string identifier;
        
};

class pokemon_stats{
    public:
        int pokemon_id,
            stat_id,
            base_stat,
            effort;
};

class pokemon_types{
    public:
        int pokemon_id,
            type_id,
            slot;
};

class pokemon_moves{
    public:
    int pokemon_id,
        version_group_id,
        move_id,
        pokemon_move_method_id,
        level,
        order;
};

class pokemon_speices{
    public:
    int id,
        generation_id,
        evolves_from_species_id,
        evolution_chain_id,
        color_id,shape_id,
        habitat_id,
        gender_rate,
        capture_rate,
        base_happiness,
        is_baby,
        hatch_counter,
        has_gender_differences,
        growth_rate_id,
        forms_switchable,
        is_legendary,
        is_mythical,
        order,
        conquest_order;

    string identifier;
};

class moves{
    public:
        int id,
        generation_id,
        type_id,
        power,
        pp,
        accuracy,
        priority,
        target_id,
        damage_class_id,
        effect_id,
        effect_chance,
        contest_type_id,
        contest_effect_id,
        super_contest_effect_id;
        
        string identifier;
};

class experance{
    public:
        int growth_rate_id, 
        level, 
        experience;
};

class stats{
    public:
        int id, 
            damage_class_id,
            is_battle_only,
            game_index;

    string identifier;
};

class type_names{
    public:
    int type_id, 
        local_language_id;

    string name;
};

class database{
    public:
        pokemon pokemonl[1092];
        pokemon_types pokemon_typesl[1675];
        pokemon_stats pokemon_statsl[6552];
        pokemon_speices pokemon_speicesl[898];
        pokemon_moves pokemon_movesl[528238];
        moves movesl[844];
        experance expl[600];
        stats statsl[8];
        type_names type_namesl[193];
};


void pokemon_driver(const char *path, database *db); //
void pokemon_types_driver(const char *path, database *db); //
void pokemon_stats_driver(const char *path, database *db); //
void pokemon_moves_driver(const char *path, database *db); //
void pokemon_speices_driver(const char *path, database *db); //

void exp_driver(const char *path, database *db);
void moves_driver(const char *path, database *db);
void stats_driver(const char *path, database *db);
void type_names_driver(const char *path, database *db);

int database_init(database *db);

#endif