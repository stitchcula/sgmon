/*
* mark that:readelf -d build/Release/sgmon.node |grep ORIGIN
*/

#include <node.h>
#include <v8.h>
#include <uv.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <dlfcn.h>

#include "sgmon.h"
#include "statgrab.h"

using namespace v8;

struct reqData{
  int delay;
  Isolate* isolate;
  Persistent<Function> callback;
};

typedef struct{
  sg_cpu_percents *cpu_percents;
  sg_mem_stats *mem_stats;
  sg_swap_stats *swap_stats;
  sg_load_stats *load_stats;
  sg_process_count *process_count;
  sg_page_stats *page_stats;

  sg_network_io_stats *network_io_stats;
  size_t network_io_entries;

  sg_disk_io_stats *disk_io_stats;
  size_t disk_io_entries;

  sg_fs_stats *fs_stats;
  size_t fs_entries;

  sg_host_info *host_info;
  sg_user_stats *user_stats;
  size_t user_entries;
} stats_t;

static stats_t stats;
static reqData mReqData;
static bool inited=false;

static int get_stats(void) {
  stats.cpu_percents = sg_get_cpu_percents(NULL);
  stats.mem_stats = sg_get_mem_stats(NULL);
  stats.swap_stats = sg_get_swap_stats(NULL);
  stats.load_stats = sg_get_load_stats(NULL);
  stats.process_count = sg_get_process_count();
  stats.page_stats = sg_get_page_stats_diff(NULL);
  stats.network_io_stats = sg_get_network_io_stats_diff(&(stats.network_io_entries));
  stats.disk_io_stats = sg_get_disk_io_stats_diff(&(stats.disk_io_entries));
  stats.fs_stats = sg_get_fs_stats(&(stats.fs_entries));
  stats.host_info = sg_get_host_info(NULL);
  stats.user_stats = sg_get_user_stats(&stats.user_entries);

  return 1;
}

void obj_build(Isolate* isolate,Local<Object> obj){

  sg_disk_io_stats *disk_io_stat_ptr;
  sg_network_io_stats *network_stat_ptr;

  Local<Object> host_info = Object::New(isolate);
  host_info->Set(String::NewFromUtf8(isolate, "hostname"),
    String::NewFromUtf8(isolate, stats.host_info->hostname));
  host_info->Set(String::NewFromUtf8(isolate, "uptime"),
    Number::New(isolate,(int)stats.host_info->uptime));

  Local<Object> load_stats = Object::New(isolate);
  load_stats->Set(String::NewFromUtf8(isolate, "min1"),
    Number::New(isolate,stats.load_stats->min1));
  load_stats->Set(String::NewFromUtf8(isolate, "min5"),
    Number::New(isolate,stats.load_stats->min5));
  load_stats->Set(String::NewFromUtf8(isolate, "min15"),
    Number::New(isolate,stats.load_stats->min15));

  Local<Object> cpu = Object::New(isolate);
  cpu->Set(String::NewFromUtf8(isolate, "idle"),
    Number::New(isolate,stats.cpu_percents->idle));
  cpu->Set(String::NewFromUtf8(isolate, "system"),
    Number::New(isolate,stats.cpu_percents->kernel + stats.cpu_percents->iowait + stats.cpu_percents->swap));
  cpu->Set(String::NewFromUtf8(isolate, "user"),
    Number::New(isolate,stats.cpu_percents->user + stats.cpu_percents->nice));

  Local<Object> process = Object::New(isolate);
  process->Set(String::NewFromUtf8(isolate, "running"),
    Number::New(isolate,stats.process_count->running));
  process->Set(String::NewFromUtf8(isolate, "zombie"),
    Number::New(isolate,stats.process_count->zombie));
  process->Set(String::NewFromUtf8(isolate, "sleeping"),
    Number::New(isolate,stats.process_count->sleeping));
  process->Set(String::NewFromUtf8(isolate, "total"),
    Number::New(isolate,stats.process_count->total));
  process->Set(String::NewFromUtf8(isolate, "stopped"),
    Number::New(isolate,stats.process_count->stopped));  

  Local<Object> mem = Object::New(isolate);
  mem->Set(String::NewFromUtf8(isolate, "total"),
    Number::New(isolate,stats.mem_stats->total));
  mem->Set(String::NewFromUtf8(isolate, "used"),
    Number::New(isolate,stats.mem_stats->used));
  mem->Set(String::NewFromUtf8(isolate, "free"),
    Number::New(isolate,stats.mem_stats->free));

  Local<Object> swap = Object::New(isolate);
  swap->Set(String::NewFromUtf8(isolate, "total"),
    Number::New(isolate,stats.swap_stats->total));
  swap->Set(String::NewFromUtf8(isolate, "used"),
    Number::New(isolate,stats.swap_stats->used));
  swap->Set(String::NewFromUtf8(isolate, "free"),
    Number::New(isolate,stats.swap_stats->free));

  Local<Array> disk = Array::New(isolate);
  disk_io_stat_ptr = stats.disk_io_stats;
  for(unsigned int i=0;i<stats.disk_io_entries;i++){
    Local<Object> disk_item = Object::New(isolate);
    disk_item->Set(String::NewFromUtf8(isolate, "name"),
      String::NewFromUtf8(isolate, disk_io_stat_ptr->disk_name));
    disk_item->Set(String::NewFromUtf8(isolate, "read"),
      Number::New(isolate, (disk_io_stat_ptr->systime)? (disk_io_stat_ptr->read_bytes/disk_io_stat_ptr->systime): disk_io_stat_ptr->read_bytes));
    disk_item->Set(String::NewFromUtf8(isolate, "write"),
      Number::New(isolate, (disk_io_stat_ptr->systime)? (disk_io_stat_ptr->write_bytes/disk_io_stat_ptr->systime): disk_io_stat_ptr->write_bytes));
    disk->Set(i, disk_item);
    disk_io_stat_ptr++;
  }

  Local<Array> network = Array::New(isolate);
  network_stat_ptr = stats.network_io_stats;
  for(unsigned int i=0;i<stats.network_io_entries;i++){
    Local<Object> network_item = Object::New(isolate);
    network_item->Set(String::NewFromUtf8(isolate, "interface"),
      String::NewFromUtf8(isolate, network_stat_ptr->interface_name));
    network_item->Set(String::NewFromUtf8(isolate, "rx"),
      Number::New(isolate, (network_stat_ptr->systime)? (network_stat_ptr->rx / network_stat_ptr->systime): network_stat_ptr->rx));
    network_item->Set(String::NewFromUtf8(isolate, "tx"),
      Number::New(isolate, (network_stat_ptr->systime)? (network_stat_ptr->tx / network_stat_ptr->systime): network_stat_ptr->tx));
      network->Set(i,network_item);
      network_stat_ptr++;
  }

  obj->Set(String::NewFromUtf8(isolate, "host_info"),host_info);
  obj->Set(String::NewFromUtf8(isolate, "load_stats"),load_stats);
  obj->Set(String::NewFromUtf8(isolate, "cpu"),cpu);
  obj->Set(String::NewFromUtf8(isolate, "mem"),mem);
  obj->Set(String::NewFromUtf8(isolate, "swap"),swap);
  obj->Set(String::NewFromUtf8(isolate, "disk"),disk);
  obj->Set(String::NewFromUtf8(isolate, "network"),network);
}

void timer_cb(uv_timer_t *handle){
  Isolate* isolate =mReqData.isolate;
  HandleScope scope(isolate);

  get_stats();
  bool error=false;

  Local<Value> argv[2];
  if(error){
    argv[0]=Exception::Error(String::NewFromUtf8(isolate,"Something went wrong!"));
    argv[1]=Undefined(isolate);    
  }else{
    argv[0]=Null(isolate);
    
    Local<Object> obj = Object::New(isolate);
    obj_build(isolate,obj);
    argv[1]=obj;
  }
  Local<Function> callback=PersistentToLocal(isolate,mReqData.callback);
  callback->Call(Null(isolate),2,argv);
}

/*
*invoke libuv end*/

void nap(const FunctionCallbackInfo<Value>& args) {

  Isolate* isolate = args.GetIsolate();

  // check args
  if (!args[0]->IsFunction()) {
    isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate,"_(:зゝ∠)_ Argument [callback] must be a function!")));
    return;
  }
  int delay=args[1]->IsNumber()?args[0]->NumberValue():3000;

  // callback, convert by Cast()
  Local<Function> callback = Local<Function>::Cast(args[1]);

  mReqData.callback.Reset(isolate,callback);
  mReqData.isolate=isolate;
  mReqData.delay=delay;

  //init sg
  if(!inited)
    sg_init(1);

  //uv
  uv_timer_t timer;
  uv_loop_t *loop=uv_default_loop();

  uv_timer_init(loop,&timer);
  uv_timer_start(&timer, timer_cb, 0, delay);
  uv_run(loop, UV_RUN_DEFAULT);
}

void grab(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();

  //init sg
  if(!inited)
    sg_init(1);
  get_stats();

  Local<Object> obj = Object::New(isolate);
  obj_build(isolate,obj);

  args.GetReturnValue().Set(obj);
}

void RegisterModule(Local<Object> exports, Local<Object> module) {
  NODE_SET_METHOD(exports, "nap", nap);
  NODE_SET_METHOD(exports, "grab", grab);
}

NODE_MODULE(sgmon, RegisterModule)
