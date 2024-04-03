#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>


#include "parser.h" 
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

int database_init(database *db){
    // const char *path;

    const char * path;

    if(fs::exists("/Users/haakon/Documents/327/pokegameCSV/pokedex/data/csv")){
        cout << "trying local" << endl;
        path = "/Users/haakon/Documents/327/pokegameCSV/pokedex/data/csv";
    }
    else if(fs::exists(("/share/cs327/pokedex/pokedex/data/csv"))){
        cout << "trying share file" << endl;
        path = "/share/cs327/pokedex/pokedex/data/csv";
    }
    else if(fs::exists("/.poke327/.")){
        cout << "trying /.poke327/." << endl;
        path = "/.poke327/.";
    }
    else{
        cerr << "Error: Could not find CSV files!" << endl;
    }

    // if(getenv("HOME/.poke327/.") != NULL){
    //     path = getenv("HOME/.poke327/.");
    //     cout << path << endl;
    // }
    // else if(getenv("/share/cs327/pokedex/pokedex/data/csv") != NULL){

    //     path = getenv("/share/cs327/pokedex/pokedex/data/csv");

    //     cout << path << endl;
    // }
    // else if(getenv("/Users/haakon/Documents/327/pokegameCSV/pokedex/data/csv") != NULL){
    //     path = getenv("/Users/haakon/Documents/327/pokegameCSV/pokedex/data/csv");

    //     cout << path << endl;
    // }


    pokemon_driver(path, db);
    pokemon_types_driver(path, db);
    pokemon_stats_driver(path, db);
    pokemon_moves_driver(path, db);
    pokemon_speices_driver(path, db);

    exp_driver(path, db);
    moves_driver(path, db);
    stats_driver(path, db);
    type_names_driver(path, db);
    
    return 0;
}

void pokemon_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/pokemon.csv";

    string line = "";
    input.open(p);
    getline(input, line);
    line = "";

    while(getline(input, line)){

        int ID, 
            species_id, 
            height, 
            weight,
            base_experience, 
            order,
            is_default;

        string identifier, tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        ID = atoi(tempString.c_str());
        //cout << ID << endl;

        getline(inputString, identifier, ',');
        
        getline(inputString, tempString, ',');
        species_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        height = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        weight = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        base_experience = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        order = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        is_default = atoi(tempString.c_str());
        
        db->pokemon[i].ID = ID;
        db->pokemon[i].identifier = identifier;
        db->pokemon[i].species_id = species_id;
        db->pokemon[i].height = height;
        db->pokemon[i].weight = weight;
        db->pokemon[i].base_experience = base_experience;
        db->pokemon[i].order = order;
        db->pokemon[i].is_default = is_default;

        line = "";
        i++;
    }

    // for(i = 0; i < 1092; i++){
    //     cout << "ID:" << db->pokemon[i].ID << " Name:" << db->pokemon[i].identifier << endl;
    // }
}

void pokemon_types_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/pokemon_types.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        int pokemon_id, 
            type_id, 
            slot;

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        pokemon_id = atoi(tempString.c_str());
        
        getline(inputString, tempString, ',');
        type_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        slot = atoi(tempString.c_str());
        
        db->pokemon_types[i].pokemon_id = pokemon_id;
        db->pokemon_types[i].type_id = type_id;
        db->pokemon_types[i].slot = slot;

        line = "";
        i++;
    }

    // for(i = 0; i < 1675; i++){
    //     cout << "ID: " << db->pokemon_types[i].pokemon_id << endl;
    // }
}

void pokemon_stats_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/pokemon_stats.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        int pokemon_id, 
            stat_id, 
            base_stat,
            effort;

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        pokemon_id = atoi(tempString.c_str());
        
        getline(inputString, tempString, ',');
        stat_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        base_stat = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        effort = atoi(tempString.c_str());
        
        db->pokemon_stats[i].pokemon_id = pokemon_id;
        db->pokemon_stats[i].stat_id = stat_id;
        db->pokemon_stats[i].base_stat = base_stat;
        db->pokemon_stats[i].effort = effort;

        line = "";
        i++;
    }

    // for(i = 0; i < 1675; i++){
    //     cout << "ID: " << db->pokemon_types[i].pokemon_id << endl;
    // }
}

void pokemon_moves_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/pokemon_moves.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        int pokemon_id, 
            version_id, 
            move_id,
            p_move_method_id,
            level,
            order = INT_MAX;

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        pokemon_id = atoi(tempString.c_str());
        
        getline(inputString, tempString, ',');
        version_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        move_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        p_move_method_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        level = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            order = atoi(tempString.c_str());
        }


        db->pokemon_moves[i].pokemon_id = pokemon_id;
        db->pokemon_moves[i].version_group_id = version_id;
        db->pokemon_moves[i].move_id = move_id;
        db->pokemon_moves[i].pokemon_move_method_id = p_move_method_id;
        db->pokemon_moves[i].level = level;
        db->pokemon_moves[i].order = order;

        line = "";
        i++;
    }

    // for(i = 0; i < 528238; i++){
    //     cout << "ID: " << db->pokemon_moves[i].pokemon_id << " Order: " << db->pokemon_moves[i].order << endl;
    // }
}

void pokemon_speices_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/pokemon_species.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        int pokemon_id, 
            gene_id, 
            evol_from_id = INT_MAX,
            evolution_chain_id,
            color_id,
            shape_id,
            habitat_id,
            gender_rate,
            capture_rate,
            base_happy,
            is_baby,
            hatch_couunt,
            gender_diff,
            grow_rate,
            forms_swtich,
            is_leg,
            is_myth,
            order,
            conquest_order = INT_MAX;

        string tempString, identifier;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        pokemon_id = atoi(tempString.c_str());
        
        getline(inputString, identifier, ',');

        getline(inputString, tempString, ',');
        gene_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
        evol_from_id = atoi(tempString.c_str());
        }

        getline(inputString, tempString, ',');
        evolution_chain_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        color_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        shape_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        habitat_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        gender_rate = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        capture_rate = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        base_happy = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        is_baby = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        hatch_couunt = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        gender_diff = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        grow_rate = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        forms_swtich = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        is_leg = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        is_myth = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        order = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            conquest_order = atoi(tempString.c_str());
        }


        db->pokemon_speices[i].id = pokemon_id;
        db->pokemon_speices[i].identifier = identifier;
        db->pokemon_speices[i].generation_id = gene_id;
        db->pokemon_speices[i].evolves_from_species_id = evol_from_id;
        db->pokemon_speices[i].evolution_chain_id = evolution_chain_id;
        db->pokemon_speices[i].color_id = color_id;
        db->pokemon_speices[i].shape_id = shape_id;
        db->pokemon_speices[i].habitat_id = habitat_id;
        db->pokemon_speices[i].gender_rate = gender_rate;
        db->pokemon_speices[i].capture_rate = capture_rate;
        db->pokemon_speices[i].base_happiness = base_happy;
        db->pokemon_speices[i].is_baby = is_baby;
        db->pokemon_speices[i].hatch_counter = hatch_couunt;
        db->pokemon_speices[i].has_gender_differences = gender_diff;
        db->pokemon_speices[i].growth_rate_id = grow_rate;
        db->pokemon_speices[i].forms_switchable = forms_swtich;
        db->pokemon_speices[i].is_legendary = is_leg;
        db->pokemon_speices[i].is_mythical = is_myth;
        db->pokemon_speices[i].conquest_order = conquest_order;
        db->pokemon_speices[i].order = order;

        line = "";
        i++;
    }
}

void exp_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/experience.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        db->exp[i].growth_rate_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->exp[i].level = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->exp[i].experience = atoi(tempString.c_str());

        line = "";
        i++;
    }
}

void moves_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/moves.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        db->moves[i].id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].identifier = tempString;

        getline(inputString, tempString, ',');
        db->moves[i].generation_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].type_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            db->moves[i].power = atoi(tempString.c_str());
        }
        else{
            db->moves[i].power = INT_MAX;
        }

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            db->moves[i].pp = atoi(tempString.c_str());
        }
        else{
            db->moves[i].pp = INT_MAX;
        }

        if(atoi(tempString.c_str())){
            db->moves[i].accuracy = atoi(tempString.c_str());
        }
        else{
            db->moves[i].accuracy = INT_MAX;
        }

        getline(inputString, tempString, ',');
        db->moves[i].priority = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].target_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].damage_class_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].effect_id = atoi(tempString.c_str());

        if(atoi(tempString.c_str())){
            db->moves[i].effect_chance = atoi(tempString.c_str());
        }
        else{
            db->moves[i].effect_chance = INT_MAX;
        }

        getline(inputString, tempString, ',');
        db->moves[i].contest_type_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].contest_effect_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->moves[i].super_contest_effect_id = atoi(tempString.c_str());

        line = "";
        i++;
    }
}

void stats_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/stats.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        string tempString;

        stringstream inputString(line);


        getline(inputString, tempString, ',');
        db->stats[i].id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            db->stats[i].damage_class_id = atoi(tempString.c_str());
        }
        else{
            db->stats[i].damage_class_id = INT_MAX;
        }

        getline(inputString, tempString, ',');
        db->stats[i].identifier = tempString;

        getline(inputString, tempString, ',');
        db->stats[i].is_battle_only = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        if(atoi(tempString.c_str())){
            db->stats[i].game_index = atoi(tempString.c_str());
        }
        else{
            db->stats[i].game_index = INT_MAX;
        }

        line = "";
        i++;
    }
}

void type_names_driver(const char *path, database *db){
    ifstream input;
    int i = 0;
    string p = path;
    p += "/type_names.csv";

    string line = "";
    input.open(p);

    getline(input, line);//skip line 0
    line = "";
    
    while(getline(input, line)){

        string tempString;

        stringstream inputString(line);

        getline(inputString, tempString, ',');
        db->type_names[i].type_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->type_names[i].local_language_id = atoi(tempString.c_str());

        getline(inputString, tempString, ',');
        db->type_names[i].name = tempString;

        line = "";
        i++;
    }
}
