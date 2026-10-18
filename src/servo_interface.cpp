#include "servo_moteur/servo_interface.hpp"
#include <cmath>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace servo_moteur
{

hardware_interface::CallbackReturn ServoInterface::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) !=
      hardware_interface::CallbackReturn::SUCCESS)
  {
    return hardware_interface::CallbackReturn::ERROR;
  }

  device_ = info_.hardware_parameters["device"];
  
  hw_positions_.resize(info_.joints.size(), 0.0);
  hw_commands_.resize(info_.joints.size(), 0.0);

  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn ServoInterface::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  try {
    serial_port_.Open(device_);
    serial_port_.SetBaudRate(LibSerial::BaudRate::BAUD_115200);
    serial_port_.SetCharacterSize(LibSerial::CharacterSize::CHAR_SIZE_8);
    serial_port_.SetParity(LibSerial::Parity::PARITY_NONE);
    serial_port_.SetStopBits(LibSerial::StopBits::STOP_BITS_1);
    serial_port_.SetFlowControl(LibSerial::FlowControl::FLOW_CONTROL_NONE);
  } catch (...) {
    RCLCPP_FATAL(rclcpp::get_logger("ServoHardware"), "Impossible d'ouvrir %s", device_.c_str());
    return hardware_interface::CallbackReturn::ERROR;
  }

  hw_commands_ = {0.0, 0.0};
  last_pan_cmd_ = 999.0;
  last_tilt_cmd_ = 999.0;
  return hardware_interface::CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn ServoInterface::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  if (serial_port_.IsOpen()) {
    serial_port_.Close();
  }
  return hardware_interface::CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> ServoInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i) {
    state_interfaces.emplace_back(info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_positions_[i]);
  }
  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> ServoInterface::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i) {
    command_interfaces.emplace_back(info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_commands_[i]);
  }
  return command_interfaces;
}

hardware_interface::return_type ServoInterface::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!serial_port_.IsOpen()) return hardware_interface::return_type::ERROR;

  if (serial_port_.IsDataAvailable()) {
    try {
      std::string data;
      serial_port_.ReadLine(data, '\n', 10);
      
      float pan_deg, tilt_deg;
      if (std::sscanf(data.c_str(), "P%f T%f", &pan_deg, &tilt_deg) == 2) {
        hw_positions_[0] = pan_deg * M_PI / 180.0;
        hw_positions_[1] = tilt_deg * M_PI / 180.0;
      }
    } catch (...) {}
  }
  return hardware_interface::return_type::OK;
}

hardware_interface::return_type ServoInterface::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  if (!serial_port_.IsOpen()) return hardware_interface::return_type::ERROR;

  double pan_cmd = hw_commands_[0];
  double tilt_cmd = hw_commands_[1];

  // Anti-jitter : on n'envoie à l'Arduino QUE si la commande a changé
  if (std::abs(pan_cmd - last_pan_cmd_) > 0.001 || std::abs(tilt_cmd - last_tilt_cmd_) > 0.001) 
  {
    try {
      // Conversion radians -> degrés pour l'Arduino (0 rad = 90°)
      double pan_deg = pan_cmd * 180.0 / M_PI + 90.0;
      double tilt_deg = tilt_cmd * 180.0 / M_PI + 90.0;

      std::stringstream ss;
      ss << "P" << std::fixed << std::setprecision(1) << pan_deg 
         << " T" << std::fixed << std::setprecision(1) << tilt_deg << "\n";
      
      serial_port_.Write(ss.str());
      
      // Mémorisation
      last_pan_cmd_ = pan_cmd;
      last_tilt_cmd_ = tilt_cmd;

    } catch (...) {
      return hardware_interface::return_type::ERROR;
    }
  }
  
  return hardware_interface::return_type::OK;
}

}  // namespace servo_moteur

PLUGINLIB_EXPORT_CLASS(servo_moteur::ServoInterface, hardware_interface::SystemInterface)