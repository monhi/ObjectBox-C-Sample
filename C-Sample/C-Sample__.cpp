// C-Sample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "objectbox.h"

#include "c_test_builder.h"
#include "c_test_objects.h"
#include "objectbox.h"

#define FOO_entity 1
#define FOO_prop_id 1
#define FOO_prop_text 2

#define BAR_entity 2
#define BAR_rel_foo 1
#define BAR_prop_id 1
#define BAR_prop_text 2
#define BAR_prop_id_foo 3



obx_err printError() 
{
	printf("Unexpected error: %d, %d (%s)\n", obx_last_error_code(), obx_last_error_secondary(),obx_last_error_message());
	return obx_last_error_code();
}

OBX_model* createModel() 
{
	OBX_model* model = obx_model();
	if (!model) {
		return NULL;
	}
	obx_uid uid = 1000;
	obx_uid fooUid = uid++;
	obx_uid fooIdUid = uid++;
	obx_uid fooTextUid = uid++;

	if (obx_model_entity(model, "Foo", FOO_entity, fooUid) ||
		obx_model_property(model, "id", OBXPropertyType_Long, FOO_prop_id, fooIdUid) ||
		obx_model_property_flags(model, OBXPropertyFlags_ID) ||
		obx_model_property(model, "text", OBXPropertyType_String, FOO_prop_text, fooTextUid) ||
		obx_model_entity_last_property_id(model, FOO_prop_text, fooTextUid)) {
		obx_model_free(model);
		return NULL;
	}

	obx_uid barUid = uid++;
	obx_uid barIdUid = uid++;
	obx_uid barTextUid = uid++;
	obx_uid barFooIdUid = uid++;
	obx_uid relUid = uid++;
	obx_schema_id relIndex = 1;
	obx_uid relIndexUid = uid++;

	if (obx_model_entity(model, "Bar", BAR_entity, barUid) ||
		obx_model_property(model, "id", OBXPropertyType_Long, BAR_prop_id, barIdUid) ||
		obx_model_property_flags(model, OBXPropertyFlags_ID) ||
		obx_model_property(model, "text", OBXPropertyType_String, BAR_prop_text, barTextUid) ||
		obx_model_property(model, "fooId", OBXPropertyType_Relation, BAR_prop_id_foo, barFooIdUid) ||
		obx_model_property_relation(model, "Foo", relIndex, relIndexUid) ||
		obx_model_entity_last_property_id(model, BAR_prop_id_foo, barFooIdUid)) {
		obx_model_free(model);
		return NULL;
	}

	obx_model_last_relation_id(model, BAR_rel_foo, relUid);
	obx_model_last_index_id(model, relIndex, relIndexUid);
	obx_model_last_entity_id(model, BAR_entity, barUid);
	return model;
}



int main()
{
	int rc;

	obx_remove_db_files("objectbox");
	printf("Testing libobjectbox version %s, core version: %s\n", obx_version_string(), obx_version_core_string());
	printf("Result array support: %d\n", obx_has_feature(OBXFeature_ResultArray));


	OBX_store* store = NULL;
	OBX_txn* txn = NULL;
	OBX_cursor* cursor = NULL;
	OBX_cursor* cursor_bar = NULL;

	// Firstly, we need to create a model for our data and the store
	{
		OBX_model* model = createModel();
		if (!model) goto handle_error;

		OBX_store_options* opt = obx_opt();
		obx_opt_model(opt, model);
		store = obx_store_open(opt);
		if (!store) goto handle_error;

		// model is freed by the obx_store_open(), we can't access it anymore
	}

	txn = obx_txn_write(store);
	if (!txn) goto handle_error;

	cursor = obx_cursor(txn, FOO_entity);
	if (!cursor) goto handle_error;
	cursor_bar = obx_cursor(txn, BAR_entity);
	if (!cursor_bar) goto handle_error;

	// Clear any existing data
	if (obx_cursor_remove_all(cursor_bar)) goto handle_error;
	if (obx_cursor_remove_all(cursor)) goto handle_error;


	if ((rc = testCursorStuff(cursor))) goto handle_error;

	if ((rc = testFlatccRoundtrip())) goto handle_error;
	if ((rc = testPutAndGetFlatObjects(cursor))) goto handle_error;

	// TODO fix double close in handle_error if a close returns an error
	if (obx_cursor_close(cursor)) goto handle_error;
	if (obx_cursor_close(cursor_bar)) goto handle_error;
	if (obx_txn_success(txn)) goto handle_error;
	if (!obx_store_await_async_completion(store)) goto handle_error;
	if (obx_store_close(store)) goto handle_error;

	return 0;

	// print error and cleanup on error
handle_error:
	if (!rc) rc = -1;
	printError();

	// cleanup anything remaining
	if (cursor) {
		obx_cursor_close(cursor);
	}
	if (txn) {
		obx_txn_close(txn);
	}
	if (store) {
		obx_store_await_async_completion(store);
		obx_store_close(store);
	}
	return rc;


}

