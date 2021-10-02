#include<thread>
#include<semaphore>
#include<memory>
#include<chrono>
#include<iostream>
#pragma once

#define MAX_ITEM	((int)1e7)
#define WAIT_TIME	500

template<typename T>
class thread_safe_queue
{
	typedef struct node {
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
		node() : data(nullptr), next(nullptr){}
		node(T& const data_ ): data(std::make_shared<T>(std::move(data_))), next(nullptr){}
	};
	std::unique_ptr<node> head;
	node* tail;
	std::binary_semaphore head_bsema, tail_bsema;
	std::counting_semaphore<MAX_ITEM> head_sema;
public:
	thread_safe_queue() : head (new node), tail(head.get()),
		head_bsema{ std::binary_semaphore{1} },
		tail_bsema{ std::binary_semaphore{1} },
		head_sema{std::counting_semaphore<MAX_ITEM>{0}}
	{}

	void push(T& const val) 
	{
		std::unique_ptr<node> new_tail(new node);
		tail_bsema.acquire();
		tail->data = std::make_shared<T>(std::move(val));
		node* const new_tail_ptr = new_tail.get();
		tail->next = std::move(new_tail);
		tail = new_tail_ptr;
		tail_bsema.release();
		head_sema.release();
	}
	
	std::shared_ptr<T> pop() {
		while (!head_sema.try_acquire_for(std::chrono::milliseconds(WAIT_TIME))) {
			//Do something if the no items in Q and waited for WAIT_TIME ms
			std::cerr <<"Thread :" <<std::this_thread::get_id() << ", Queue Empty, Please Refill\n";
		}
		head_bsema.acquire();
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		head_bsema.release();
		return old_head->data;
	}
};