#include "glog/logging.h"
#include "common/utils/file/file.h"
#include "homework5/display/main_window.h"
#include "gflags/gflags.h"
#include "homework5/map/map_lib.h"
#include "homework5/route/find_route.h"

DEFINE_string(route_file_path, "", "Path of displayed route");
DEFINE_string(map_dir, "", "Directory path of map file");

int main(int argc, char* argv[]) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  
  
  //find_pred_succ(); puts("pred_succ"); return 0;
  
  for (int i=1;i<=5;++i){
	  char path_src[305], path_dst[305];
	  sprintf(path_src, "/home/hqztrue/Desktop/ponyai/homework5/data/routes/route_request_%d.txt", i);
	  sprintf(path_dst, "/home/hqztrue/Desktop/ponyai/homework5/data/routes/route_result_%d.txt", i);
	  find_route(path_src, path_dst);
	  puts("-------------");
  }
  //puts("find_route");
  return 0;
}


