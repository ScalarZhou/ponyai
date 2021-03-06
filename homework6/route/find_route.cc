#include "homework6/route/find_route.h"

bool point_equal(const interface::geometry::Point3D &p1, const interface::geometry::Point3D &p2){
	const double eps = 1e-3;
	return fabs(p1.x()-p2.x())<eps && fabs(p1.y()-p2.y())<eps && fabs(p1.z()-p2.z())<eps;
}

void find_pred_succ(){
	homework6::map::MapLib map_lib;
	interface::map::Map map = map_lib.map_proto();
	int n = map.lane_size();
	for (int i=0;i<n;++i){
		//interface::map::Lane &lane1 = map.lane(i);
		for (int j=0;j<n;++j)
			if (i!=j){
				//interface::map::Lane &lane2 = map.lane(j);
				//check lane(i)->lane(j), by the ends of central_line
				if (point_equal(map.lane(i).central_line().point(map.lane(i).central_line().point_size()-1),map.lane(j).central_line().point(0))){
						interface::map::Id *pid = map.mutable_lane(i)->add_successor();
						*pid = map.lane(j).id();
						pid = map.mutable_lane(j)->add_predecessor();
						*pid = map.lane(i).id();
					}
			}
	}
	CHECK(file::WriteProtoToTextFile(map, (pony_root+"homework6/processed_map_proto.txt").c_str()));
}

int location(const interface::map::Lane &lane, const interface::geometry::Point2D &pt){  //project a point to a lane's central_line, by finding nearest neighbor
	int n = lane.central_line().point_size(), id = 0;
        double dmin = 1e10;
	geometry::point p(pt.x(), pt.y());
	for (int i=0;i<n;++i){
		interface::geometry::Point3D p1 = lane.central_line().point(i);
		double d = p.dist(geometry::point(p1.x(), p1.y()));
		if (d<dmin)dmin=d, id = i;
	}
	return id;
}

double cal_len(const interface::map::Lane &lane, int sta=0, int end=-1){  //calculate the length of part of a lane [sta,end), by central_line
	if (end==-1)end = lane.central_line().point_size();
	double l=0;
	for (int j=sta;j<end-1;++j){
		interface::geometry::Point3D pt = lane.central_line().point(j);
		geometry::point p1(pt.x(), pt.y());
		pt = lane.central_line().point(j+1);
		geometry::point p2(pt.x(), pt.y());
		l+=p1.dist(p2);
	}
	return l;
}

void add_route_point(interface::route::Route &route, const interface::map::Lane &lane, int sta=0, int end=-1){  //add[sta,end) of lane to route
	if (end==-1)end = lane.central_line().point_size();
	for (int i=sta;i<end;++i){
		interface::geometry::Point2D *p = route.add_route_point();
		p->set_x(lane.central_line().point(i).x());
        p->set_y(lane.central_line().point(i).y());
	}
}

double CalcDistance(const interface::geometry::Vector3d& position,
                      const interface::geometry::Point3D& destination) {
double sqr_sum =
	geometry::sqr(position.x() - destination.x()) + geometry::sqr(position.y() - destination.y());
return std::sqrt(sqr_sum);
}

double len2D(const interface::geometry::Vector3d& v) {  //x, y
    return std::sqrt(geometry::sqr(v.x()) + geometry::sqr(v.y()));
}

double dot2D(const interface::geometry::Vector3d& v1, const interface::geometry::Vector3d& v2){
	return v1.x()*v2.x()+v1.y()*v2.y()+v1.z()*v2.z();
}

double dist(const interface::geometry::Point2D& x, const interface::geometry::Point2D& y){
	return std::sqrt(geometry::sqr(x.x() - y.x()) + geometry::sqr(x.y() - y.y()));
}

double len(interface::route::Route &route){
	double d = 0;
	for (int i=0;i<route.route_point_size()-1;++i)
		d += dist(route.route_point(i),route.route_point(i+1));
	return d;
}

struct Node{
	double d;
	int x;
	Node *pre;
	Node(double _d,int _x,Node *_pre){d=_d;x=_x;pre=_pre;}
	friend bool operator <(const Node &x,const Node &y){return x.d>y.d;}
};

struct pNode{
	Node *p;
	pNode(Node *_p):p(_p){}
	friend bool operator <(const pNode &x,const pNode &y){return *(x.p)<*(y.p);}
};

double dist(const interface::map::Lane &lane, const geometry::point &p){
	double ans = 1e10;
	for (int i=0;i<lane.central_line().point_size();++i){
		double d = p.dist(geometry::point(lane.central_line().point(i).x(), lane.central_line().point(i).y()));
		if (d<ans)ans=d;
	}
	return ans;
}

void load_map(interface::map::Map *&pmap){
	if (pmap==NULL){
        pmap = new interface::map::Map();
		const char map_path[305] = "/home/hqztrue/Desktop/ponyai/homework5/processed_map_proto.txt";  //
		CHECK(file::ReadFileToProto(map_path, pmap));
	}
}

void distinct_point(interface::route::Route &route){
	vector<interface::geometry::Point2D> v;
	double eps = 1e-5;
	for (int i=0;i<route.route_point_size();++i)
		if (v.size()==0 || dist(v[v.size()-1], route.route_point(i))>eps){
			v.push_back(route.route_point(i));
		}
	route.clear_route_point();
	for (int i=0;i<v.size();++i){
		interface::geometry::Point2D *p = route.add_route_point();
		p->set_x(v[i].x());
        p->set_y(v[i].y());
	}
}

void find_route(interface::route::Route &route){
	route.clear_route_point();
	static interface::map::Map *pmap = NULL;
	load_map(pmap);
	interface::map::Map &map = *pmap;
	
	int n = map.lane_size();
	vector<int> start, end;
	//find lanes that contain start_point
	for (int i=0;i<n;++i){
		geometry::polygon poly;
		for (int j=0;j<map.lane(i).left_bound().boundary().point_size();++j){
			interface::geometry::Point3D p = map.lane(i).left_bound().boundary().point(j);
			poly.add(geometry::point(p.x(),p.y()));
		}
		for (int j=map.lane(i).right_bound().boundary().point_size()-1;j>=0;--j){
			interface::geometry::Point3D p = map.lane(i).right_bound().boundary().point(j);
			poly.add(geometry::point(p.x(),p.y()));
		}
		if (poly.inside(geometry::point(route.start_point().x(),route.start_point().y()))){
			start.push_back(i);
			printf("start %d\n",i);
		}
		//find lanes that contain end_point
		if (poly.inside(geometry::point(route.end_point().x(),route.end_point().y()))){
			end.push_back(i);
			printf("end %d\n",i);
		}
	}
	
	//assume point on central line: find nearest point
	double mind = 1e10;
	geometry::point p = geometry::point(route.start_point().x(),route.start_point().y());
	for (int i=0;i<start.size();++i){
		double d = dist(map.lane(start[i]), p);
		if (d<mind)mind=d, start[0]=start[i];
	}
	mind = 1e10;
	p = geometry::point(route.end_point().x(),route.end_point().y());
	for (int i=0;i<end.size();++i){
		double d = dist(map.lane(end[i]), p);
		if (d<mind)mind=d, end[0]=end[i];
	}
	assert(start.size()>0);
	assert(end.size()>0);
	start.resize(1);
	end.resize(1);
	
	//in the same lane
	for (int i=0;i<start.size();++i)
		for (int j=0;j<end.size();++j)
			if (start[i]==end[j]){
				//check start_point is before end_point
				int id1 = location(map.lane(start[i]), route.start_point());
				int id2 = location(map.lane(end[j]), route.end_point());
				if (id1<=id2){
					add_route_point(route, map.lane(start[i]), id1, id2+1);
					//CHECK(file::WriteProtoToTextFile(route, path_dst));
					distinct_point(route);return;
				}
			}
	
	//name->lane id
	std::map<string, int> M;
	for (int i=0;i<n;++i)
		M[map.lane(i).id().id()] = i;
	
	//calculate the length of each lane, by central_line
	vector<double> length(n+1, 0);
	for (int i=0;i<n;++i)
		length[i] = cal_len(map.lane(i));
	
	//dijkstra
	priority_queue<pNode> Q;
	vector<int> cnt(n+1, 0);
	for (int i=0;i<start.size();++i){
		int m=map.lane(start[i]).successor_size();
		pNode ptr_(new Node(0,start[i],NULL));
		for (int j=0;j<m;++j){
			double d = cal_len(map.lane(start[i]),location(map.lane(start[i]), route.start_point()));
			pNode ptr(new Node(d,M[map.lane(start[i]).successor(j).id()],ptr_.p));
			Q.push(ptr);
		}
	}
	while (!Q.empty()){
		Node *ptr = Q.top().p;
		double d=Q.top().p->d;
		int x=Q.top().p->x;
		cnt[x]=1;
		printf("x=%d\n",x);
		for (int i=0;i<end.size();++i)
			if (x==end[i]){
				vector<Node*> v;
				while (1){
					if (ptr->pre == NULL){
						//points in the first lane
						int id = location(map.lane(ptr->x), route.start_point());
						add_route_point(route, map.lane(ptr->x), id);
						break;
					}
					v.push_back(ptr);
					ptr = ptr->pre;
				}
				//points in the middle lanes
				for (int j=v.size()-1;j>=1;--j)
					add_route_point(route, map.lane(v[j]->x));
				
				//points in the last lane
				int id = location(map.lane(x), route.end_point());
				add_route_point(route, map.lane(x), 0, id+1);
				//CHECK(file::WriteProtoToTextFile(route, path_dst));
				distinct_point(route);return;
			}
		
		int m=map.lane(x).successor_size();
		for (int j=0;j<m;++j){
			pNode next_ptr(new Node(d+length[x],M[map.lane(x).successor(j).id()],ptr));
			Q.push(next_ptr);
		}
		
		while (!Q.empty()&&cnt[Q.top().p->x])Q.pop();
	}
	distinct_point(route);return;
}

void find_route(char path_src[], char path_dst[]){
	interface::route::Route route;
	CHECK(file::ReadFileToProto(path_src, &route));
	find_route(route);
	//print
	CHECK(file::WriteProtoToTextFile(route, path_dst));
	return;
}


