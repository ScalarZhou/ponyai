// Copyright @2018 Pony AI Inc. All rights reserved.

#include "homework4/pointcloud_viewer.h"

DEFINE_string(lidar_device, "VelodyneDevice32c", "");

PointCloudViewer::PointCloudViewer(Options options, QWidget* parent, const std::string& data_dir)
    : PainterWidgetBase(options, parent), data_dir_(data_dir) {
  const std::string pointcloud_dir = file::path::Join(data_dir_, "select", FLAGS_lidar_device);
  CHECK(file::path::Exists(pointcloud_dir)) << pointcloud_dir << " doesn't exist!";
  pointcloud_files_ = file::path::FindFilesWithPrefixSuffix(pointcloud_dir, "", "txt");
  std::sort(pointcloud_files_.begin(), pointcloud_files_.end(), file::path::Compare);

  const std::string pointcloud_label_dir =
      file::path::Join(data_dir_, "label", FLAGS_lidar_device);
  data_label_map_ = ObtainDataToLabelMapping(pointcloud_dir, pointcloud_label_dir);

  default_prism_style_.show_vertices = true;
  default_prism_style_.show_edge = true;
  default_prism_style_.show_plane = true;
  default_prism_style_.vertice_style.point_size = 4.0;
  default_prism_style_.edge_style.line_width = 2;
  default_prism_style_.plane_style.alpha = 0.2;
  default_prism_style_.set_color(utils::display::Color::Red());

  default_point_style_.point_size = 1.0;
  default_point_style_.point_color = utils::display::Color::Green();
  in_point_style_.point_size = 10.0;
  in_point_style_.point_color = utils::display::Color::Blue();

  startTimer(100);
}

void PointCloudViewer::InitializeGlPainter() {
  gl_painter_ = std::make_unique<utils::display::OpenglPainter>(gl_context(), font_renderer());
  gl_painter_->SetupOpenGL();
}

void PointCloudViewer::keyPressEvent(QKeyEvent* event) {
  const int key = event->key();
  switch (key) {
    case Qt::Key_N:
      file_index_ = (++file_index_) % pointcloud_files_.size();
      // Load pointcloud data.
      const std::string pointcloud_file = pointcloud_files_[file_index_];
      const PointCloud pointcloud_ = ReadPointCloudFromTextFile(pointcloud_file);
      CHECK(!pointcloud_.points.empty());
      LOG(INFO) << "Load pointcloud: " << pointcloud_file;
      points_.clear();
      points_in.clear();
      points_.reserve(pointcloud_.points.size());
      for (const auto& point : pointcloud_.points) {
        Eigen::Vector3d point_in_world = pointcloud_.rotation * point + pointcloud_.translation;
        points_.emplace_back(point_in_world.x(), point_in_world.y(), point_in_world.z());
      }

      // Load object label if there is a corresponding one.
      labels_.clear();
	  obstacles_.clear();
      if (data_label_map_.count(pointcloud_file)) {
        interface::object_labeling::ObjectLabels object_labels;
        CHECK(file::ReadFileToProto(data_label_map_[pointcloud_file], &object_labels));
        for (const auto& object : object_labels.object()) {
          labels_.emplace_back();
          PointCloudLabel& label = labels_.back();
          label.id = object.id();
          CHECK_GT(object.polygon().point_size(), 0);
          for (const auto& point : object.polygon().point()) {
            label.floor = std::min(label.floor, point.z());
            label.polygon.emplace_back(point.x(), point.y());
          }
          label.ceiling = label.floor + object.height();
		  
		  obstacles_.emplace_back();
		  interface::perception::PerceptionObstacle& obstacle = obstacles_.back();
		  obstacle.set_id(object.id());
		  obstacle.set_heading(object.heading());
		  obstacle.set_height(object.height());
		  for (const auto& point : object.polygon().point()) {
			interface::geometry::Point3D *ppoint = obstacle.add_polygon_point();
			ppoint->set_x(point.x());ppoint->set_y(point.y());ppoint->set_z(point.z());
          }
		  obstacle.set_type(object.type());
		  
		  for (const auto& point : points_){
        //printf("test\n");
			if (!within(point, object))continue;
      printf("in\n");
			interface::geometry::Point3D *ppoint = obstacle.add_object_points();
			ppoint->set_x(point[0]);ppoint->set_y(point[1]);ppoint->set_z(point[2]);
      points_in.push_back(point);
		  }
        }
      }

      // Update camera center.
      painter_widget_controller_->MutableCamera()->UpdateCenter(
          pointcloud_.translation.x(),
          pointcloud_.translation.y(),
          pointcloud_.translation.z());
      break;
  }
}

void PointCloudViewer::Paint3D() {
  //gl_painter()->DrawPoints<math::Vec3d>(
      //utils::ConstArrayView<math::Vec3d>(points_.data(), points_.size()), default_point_style_);
  gl_painter()->DrawPoints<math::Vec3d>(
      utils::ConstArrayView<math::Vec3d>(points_in.data(), points_in.size()), in_point_style_);
  if (!labels_.empty()) {
    for (const auto& label : labels_) {
      DrawPointCloudLabel(label);
    }
  }
}

std::unordered_map<std::string, std::string> PointCloudViewer::ObtainDataToLabelMapping(
    const std::string& data_dir, const std::string& label_dir) {
  std::unordered_map<std::string, std::string> data_to_label_map;
  // Obtain all label files.
  std::vector<std::string> label_files =
      file::path::FindFilesWithPrefixSuffix(label_dir, "", "label");
  LOG(INFO) << "Total label files found: " << label_files.size();
  // Construct data to label file map.
  for (auto const& label_file : label_files) {
    const std::string frame_id = file::path::FilenameStem(file::path::Basename(label_file));
    const std::string data_file = file::path::Join(data_dir, strings::Format("{}.txt", frame_id));
    if (file::path::Exists(data_file)) {
      data_to_label_map.emplace(data_file, label_file);
    } else {
      LOG(WARNING) << "Fail to find label for " << data_file;
    }
  }
  return data_to_label_map;
};

void PointCloudViewer::DrawPointCloudLabel(const PointCloudLabel& label) {
  font_renderer()->DrawText3D(
      label.id, math::Vec3d(label.polygon[0].x, label.polygon[0].y, label.ceiling + 0.25),
      utils::display::Color::Yellow(), "Arial", 12);

  gl_painter()->DrawPrism<math::Vec2d>(
      utils::ConstArrayView<math::Vec2d>(label.polygon.data(), label.polygon.size()),
      label.ceiling, label.floor, default_prism_style_);
}

bool within(const math::Vec3<double> &point, const interface::object_labeling::ObjectLabel &object){
	const double PI = 4*atan(1);
  double floor = std::numeric_limits<double>::infinity();
  for (const auto& p : object.polygon().point()) {
    floor = std::min(floor, p.z());
  }
  //ceiling = label.floor + object.height();
	if (point[2]<floor || point[2]>floor+object.height())return 0;
	double distl = (point[0]-object.center_x())*cos(object.heading())+(point[1]-object.center_y())*sin(object.heading());
	if (fabs(distl)>object.length()/2)return 0;
	double distw = (point[0]-object.center_x())*cos(object.heading()+PI/2)+(point[1]-object.center_y())*sin(object.heading()+PI/2);
	if (fabs(distw)>object.width()/2)return 0;
	return 1;
}


