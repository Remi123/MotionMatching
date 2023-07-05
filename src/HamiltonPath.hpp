#include <vector>
#include <utility>
#include <cmath>
#include <cstddef>
#include <random>
#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/method_bind.hpp>

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;
struct HamiltonPath : godot::Resource{
    GDCLASS(HamiltonPath,Resource);

    HamiltonPath(const int _w = 10,const int _h = 10, const int _s = 0)
    :   rows{_w},cols{_h}, steps{_s}, gen{}, data{}
    {
    }
    int rows = 0, cols = 0, steps = 0;
    bool is_confined = true;
    std::mt19937 gen;
    int seed_copy = 0;

    std::vector<Vector3i> data{};

    enum GridType {
        Orthogonal,
        Hexagonal
    };

    void set_confined(bool value){is_confined = value;}
    bool get_confined(){return is_confined;}

    GridType gridtype = GridType::Orthogonal;
    void set_gridtype(GridType value){gridtype = value;}
    GridType get_gridtype(){return gridtype;}

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("set_gridtype","type"), &HamiltonPath::set_gridtype);
        ClassDB::bind_method(D_METHOD("get_gridtype"), &HamiltonPath::get_gridtype);
        ADD_PROPERTY(PropertyInfo(Variant::INT,"Type",godot::PROPERTY_HINT_ENUM ,"Orthogonal,Hexagonal"), "set_gridtype", "get_gridtype");

        ClassDB::bind_method(D_METHOD("set_row_col","limit"), &HamiltonPath::set_row_col);
        ClassDB::bind_method(D_METHOD("get_row_col"), &HamiltonPath::get_row_col);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3I, "RowsColumns"), "set_row_col", "get_row_col");

        ClassDB::bind_method(D_METHOD("set_confined","value"), &HamiltonPath::set_confined);
        ClassDB::bind_method(D_METHOD("get_confined"), &HamiltonPath::get_confined);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "Confined"), "set_confined", "get_confined");

        ClassDB::bind_method(D_METHOD("get_seed"), &HamiltonPath::get_seed);
        ClassDB::bind_method(D_METHOD("set_seed","value"), &HamiltonPath::set_seed);
        ClassDB::bind_method(D_METHOD("reset_seed"), &HamiltonPath::reset_seed);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "seed"), "set_seed", "get_seed");
        
        ClassDB::bind_method(D_METHOD("set_data","value"), &HamiltonPath::set_data);
        ClassDB::bind_method(D_METHOD("get_data"), &HamiltonPath::get_data);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "data"), "set_data", "get_data");

        ClassDB::bind_method(D_METHOD("generate_hamilton_path","quality"), &HamiltonPath::generate_hamilton_path);

        BIND_ENUM_CONSTANT(Orthogonal);
        BIND_ENUM_CONSTANT(Hexagonal);   
    }
public :
    Vector3i get_row_col(){return Vector3i{rows,steps,cols};}
    void set_row_col(Vector3i limits){rows = abs(limits.x); steps = abs(limits.y) ;cols = abs(limits.z);}



    int get_seed(){return seed_copy;}
    void set_seed(int value){seed_copy = value;gen.seed(seed_copy);}
    void reset_seed(){gen = std::mt19937();}

    PackedVector3Array _data;
    PackedVector3Array get_data(){
        return _data;
    }
    void set_data(PackedVector3Array other){_data = other;}

    void generate_hamilton_path(const float q){
        data.clear();
        // std::uniform_int_distribution<> dis_x(0, rows);
        // std::uniform_int_distribution<> dis_y(0, steps);
        // std::uniform_int_distribution<> dis_z(0, cols);

        // data.push_back({dis_x(gen),dis_y(gen),dis_z(gen)});
        data.push_back(Vector3i{});
        possibility.clear();
        if(gridtype == HamiltonPath::GridType::Orthogonal)
        {
            UtilityFunctions::print("Grid");
            if(rows > 0)
            {
                possibility.push_back({1,0,0});
                possibility.push_back({-1,0,0});
            };
            if(steps > 0)
            {
                possibility.push_back({0,1,0});
                possibility.push_back({0,-1,0});
            };
            if(cols > 0)
            {
                possibility.push_back({0,0,1});
                possibility.push_back({0,0,-1});
            };
        }
        else if(gridtype == HamiltonPath::GridType::Hexagonal)
        {
            UtilityFunctions::print("Hexagonal");
            if(rows > 0 && cols > 0)
            {
                possibility.push_back({1,0,0});
                possibility.push_back({-1,0,0});
                possibility.push_back({0,0,1});
                possibility.push_back({0,0,-1});
                possibility.push_back({1,0,-1});
                possibility.push_back({-1,0,1});
            };
            if(steps > 0)
            {
                possibility.push_back({0,1,0});
                possibility.push_back({0,-1,0});
            }
        }
 

        size_t n = 1;
		float nattempts = q * 10.0 * (rows+1) * (steps+1)* (cols+1) * std::pow(std::log(2.+(rows+1)*(steps+1)*(cols+1)),2);
        for (size_t i=0; i<nattempts; i++)
		{
			n = backbite(n);
		}
        _data.clear();
        for(auto v : data)
            _data.append(v);
    }

private:

    void reversePath(size_t i1,size_t i2)
    {
        auto s = std::next(data.begin(),i1);
        auto e = std::next(data.begin(),i2);
        if( data.size() > i1 && data.size() > i2)
            std::reverse(s,e);
        else
            throw;
    }

    bool inSublattice(Vector3i other)
    {
        if (! is_confined) return true;

        if (gridtype == GridType::Orthogonal)
        {
            if (other.x<0) return false;
            if (other.x>rows) return false;
            if (other.y<0) return false;
            if (other.y>steps) return false;        
            if (other.z<0) return false;
            if (other.z>cols) return false;
        }
        else if (gridtype == GridType::Hexagonal)
        {
            if (steps > 0 && !(0 <= other.y && other.y <= steps)) return false;
            auto distance =  (abs(other.x - 0) 
                + abs(other.x + other.z - 0 - 0)
                + abs(other.z - 0)) / 2.0;
            if(distance > rows) return false;
        }


        return true;
    }

    std::vector<Vector3i> possibility = {};

    size_t backbite(size_t n)
    {
        //choose a random end
        //choose a random neighbour
        //check if its in the sublattice
        //check if its in the path
        //if it is - then reverse loop
        //if it is not - add it to the end
        //the right hand end is always chosen
        //Choose a random step direction
        // std::uniform_int_distribution<> dis_4(0, 5);
        std::vector<Vector3i> out{};
        std::sample(possibility.begin(),possibility.end(),std::back_inserter(out),1,gen);
        Vector3i step = out[0];
        std::uniform_int_distribution<> dis(0, 1);
        if (dis(gen) == 0  )
        {
            n = backbite_left(step);
        }
        else
        {
            n = backbite_right(step);
        }

        return n;
    }

    size_t backbite_left(Vector3i step)
    {
        // choose left hand end
        Vector3i neighbour = data.front() + step;

        // check to see if neighbour is in sublattice
        if (inSublattice(neighbour))
        {
            // Now check to see if it's already in path
            auto f = std::find(data.begin(),data.end(),neighbour);
            if (f != data.end())
            {
                std::reverse(data.begin(),f);
            }
            else
            {
                std::reverse(data.begin(),data.end());
                data.push_back(neighbour);
            }
        }
        return data.size();
    }

    size_t backbite_right(Vector3i step)
    {
        //choose right hand end
        Vector3i neighbour = data.back() + step;
        
        if (inSublattice(neighbour))
        {
            //Now check to see if it's already in path
            auto f = std::find(data.rbegin(),data.rend(),neighbour);
            if (f != data.rend())
            {
                std::reverse(data.rbegin(),f);
            }
            else
            {
                data.push_back(neighbour);
            }
        }
        return data.size();
    }

};

VARIANT_ENUM_CAST(HamiltonPath::GridType);
