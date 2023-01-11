#include "plugin.hpp"
#include "reverb_param_store.h"

std::string params_filename = asset::user("chipselect_reverb_constants.json");

unsigned loadReverbParameters(ReverbParameters& params, unsigned model_index)
{
	FILE* file = fopen(params_filename.c_str(), "r");
	json_error_t err;
	json_t* file_j = json_loadf(file, 0, &err);
	size_t models_length = json_array_size(file_j);
	if(models_length){
		model_index %= models_length;
		json_t* model_j = json_array_get(file_j, model_index);

		json_t* lengths_j = json_object_get(model_j, "lengths");
		for(int i = 0; i < 5; i++){
			json_t* lengths_i_j = json_array_get(lengths_j, i);
			params.lengths[i] = simd::float_4(
			json_real_value(json_array_get(lengths_i_j, 0)), 
			json_real_value(json_array_get(lengths_i_j, 1)), 
			json_real_value(json_array_get(lengths_i_j, 2)), 
			json_real_value(json_array_get(lengths_i_j, 3)));
			json_decref(lengths_i_j);
		}
		json_decref(lengths_j);

		json_t* normals_j = json_object_get(model_j, "mixer_normals");
		for(int i = 0; i < 5; i++){
			json_t* normals_i_j = json_array_get(normals_j, i);
			params.normals[i] = simd::float_4(
			json_real_value(json_array_get(normals_i_j, 0)), 
			json_real_value(json_array_get(normals_i_j, 1)), 
			json_real_value(json_array_get(normals_i_j, 2)), 
			json_real_value(json_array_get(normals_i_j, 3)));
			json_decref(normals_i_j);
		}
		json_decref(normals_j);
		json_decref(lengths_j);
		json_decref(model_j);
		json_decref(file_j);
		return model_index;
	}
	json_decref(file_j);
	return 0;
}

void storeReverbParameters(ReverbParameters& params)
{
    json_t* lengths_arr_j = json_array();
	for(int i = 0; i < 5; i++){
		json_t* lengths_i_arr_j = json_array();
		json_array_append_new(lengths_i_arr_j, json_real(params.lengths[i][0]));
		json_array_append_new(lengths_i_arr_j, json_real(params.lengths[i][1]));
		json_array_append_new(lengths_i_arr_j, json_real(params.lengths[i][2]));
		json_array_append_new(lengths_i_arr_j, json_real(params.lengths[i][3]));
		json_array_append(lengths_arr_j, lengths_i_arr_j);
		json_decref(lengths_i_arr_j);
	}

	json_t* mixer_arr_j = json_array();
	for(int i = 0; i < 5; i++){
		json_t* mixer_i_arr_j = json_array();
		json_array_append_new(mixer_i_arr_j, json_real(params.normals[i][0]));
		json_array_append_new(mixer_i_arr_j, json_real(params.normals[i][1]));
		json_array_append_new(mixer_i_arr_j, json_real(params.normals[i][2]));
		json_array_append_new(mixer_i_arr_j, json_real(params.normals[i][3]));
		json_array_append(mixer_arr_j, mixer_i_arr_j);
		json_decref(mixer_i_arr_j);
	}

	json_t* params_j = json_object();
	json_object_set(params_j, "lengths", lengths_arr_j);
	json_object_set(params_j, "mixer_normals", mixer_arr_j);

	FILE* file = fopen(params_filename.c_str(), "r");
	if(file){
		json_error_t err;
		json_t* file_j = json_loadf(file, 0, &err);
		size_t models_length = json_array_size(file_j);
		if(models_length){
			json_array_append(file_j, params_j);
		}
		else{
			json_decref(file_j);
			file_j = json_array();
			json_array_append(file_j, params_j);
		}
		fclose(file);
		file = fopen(params_filename.c_str(), "w");
		json_dumpf(file_j, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
	json_decref(lengths_arr_j);
	json_decref(mixer_arr_j);
	json_decref(params_j);
}