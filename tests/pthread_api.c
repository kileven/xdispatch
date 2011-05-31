/*
* Copyright (c) 2008-2009 Apple Inc. All rights reserved.
* Copyright (c) 2011 MLBA. All rights reserved.
*
* @MLBA_OPEN_LICENSE_HEADER_START@
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* @MLBA_OPEN_LICENSE_HEADER_END@
*/

#include <stdlib.h>

#include "../core/platform/platform.h"
#include "tests.h"


/*
Test ensuring the proper functionality of the pthreads api on this platform
(At least as far as this is possible within a short unittest)
 */

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static int s_cter = 0;

static void* increment(void* unused){
	MU_ASSERT_NULL(pthread_mutex_lock(&s_mutex));
	MU_MESSAGE("Working");
	s_cter++;
	MU_ASSERT_NULL(pthread_mutex_unlock(&s_mutex));

	return NULL;
}

#define M_THREAD_NO 100

static void test_mutex(){
	pthread_mutex_t mutex;
	pthread_t threads[M_THREAD_NO];
	int i = 0;

	// test all operations
	MU_ASSERT_NULL(pthread_mutex_init(&mutex, NULL));
	MU_ASSERT_NULL(pthread_mutex_lock(&mutex));
	MU_ASSERT_NULL(pthread_mutex_unlock(&mutex));
	MU_ASSERT_NULL(pthread_mutex_destroy(&mutex));

	// test using static initializer
	MU_ASSERT_NULL(pthread_mutex_lock(&s_mutex));
	MU_ASSERT_NULL(pthread_mutex_unlock(&s_mutex));

	// test using various workers
	for(i = 0; i < M_THREAD_NO; i++)
		pthread_create(threads+i, NULL, increment, NULL);
	for(i = 0; i < M_THREAD_NO; i++)
		pthread_join(threads[i], NULL);
	MU_DESC_ASSERT_EQUAL("mutex_t operations", s_cter, M_THREAD_NO);
}

/*
 Test the functionality of condition variables
 */



void* cond_worker(void* unused){
	return NULL;
}

static void test_condition(){
	pthread_cond_t priv_cond;

	// test api
	MU_ASSERT_NULL(pthread_cond_init(&priv_cond, NULL));
	MU_ASSERT_NULL(pthread_cond_signal(&priv_cond));
	MU_ASSERT_NULL(pthread_cond_broadcast(&priv_cond));
}

/*
 Test thread local storage
 */

#define TLS_THREAD_NO 10

static pthread_key_t tls_key;
static char tls_visited[TLS_THREAD_NO];
static char tls_deleted[TLS_THREAD_NO];

static void* dls_worker(void* dt){
	int no = *(int*)(dt);

	MU_ASSERT_NOT_NULL_HEX(dt);

	tls_visited[no] = no;
	MU_DESC_ASSERT_NULL("pthread_setspecific", pthread_setspecific(tls_key, dt));
	MU_DESC_ASSERT_EQUAL("pthread_getspecific", *(int*)pthread_getspecific(tls_key), no);

	return NULL;
}

static void dls_destructor(void* dt){
	int no = *(int*)(dt);

	MU_ASSERT_NOT_NULL_HEX(dt);

	tls_deleted[no] = 1;
	free(dt);
}

static void test_tls(){
	pthread_t threads[TLS_THREAD_NO];
	int* curr;
	int i = 0;

	// ensure the api works
	MU_DESC_ASSERT_NULL("pthread_key_create", pthread_key_create(&tls_key, dls_destructor));
	MU_DESC_ASSERT_NULL("pthread_setspecific", pthread_setspecific(tls_key, (void*)3));
	MU_DESC_ASSERT_EQUAL("pthread_getspecific", pthread_getspecific(tls_key), (void*)3);

	// ensure each thread has its own storage
	for(i = 0; i < TLS_THREAD_NO; i++){
		tls_deleted[i] = tls_visited[i] = 0;
		curr = (int*)malloc(sizeof(int));
		MU_ASSERT_NOT_NULL_HEX(curr);
		*curr = i;
		pthread_create(threads+i, NULL, dls_worker, curr);
	}
	for(i = 0; i < TLS_THREAD_NO; i++){
		pthread_join(threads[i], NULL);
		MU_ASSERT_EQUAL(tls_visited[i], i);
		MU_ASSERT_TRUE(tls_deleted[i]);
	}
}

/*
 Test the creation of threads
 */
static void* test_thread_worker(void* dt){
	int* visited = (int*)(dt);

	*visited = 1;

	return NULL;
}

static void* test_thread_sleeper(void* dt){
	int* visited = (int*)(dt);

	pthread_exit(0);

	*visited = 1;

	return NULL;
}

static void test_thread_creation(){
	pthread_t tid;
	int visited = 0;
	int i = 0;

	// simple creation test
	pthread_create(&tid, NULL, test_thread_worker, &visited);
	MU_SLEEP(1);
	MU_DESC_ASSERT_EQUAL("pthread_create", visited, 1);

	// simple join test
	for(i = 0; i < 4; i++){
		visited = 0;
		pthread_create(&tid, NULL, test_thread_worker, &visited);
		pthread_join(tid, NULL);
		MU_DESC_ASSERT_EQUAL("pthread_join", visited, 1);
	}

	// simple exit test
	visited = 0;
	pthread_create(&tid, NULL, test_thread_sleeper, &visited);
	pthread_join(tid, NULL);
	MU_DESC_ASSERT_EQUAL("pthread_exit", visited, 0);
}

void pthread_api(){
	MU_BEGIN_TEST(pthread_api);

	//test_thread_creation();
	//test_mutex();
	test_condition();
	//test_tls();
	MU_PASS("");

	MU_END_TEST;
}
