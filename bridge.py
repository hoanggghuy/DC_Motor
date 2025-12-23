import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
import serial
import time

# --- CẤU HÌNH ROBOT (ĐO VÀ SỬA LẠI SỐ NÀY) ---
# Khoảng cách giữa 2 tâm bánh xe (mét)
WHEEL_DIST = 0.20  

class SerialBridge(Node):
    def __init__(self):
        super().__init__('serial_bridge_node')
        
        # 1. Kết nối Serial
        try:
            self.ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
            self.get_logger().info('✅ Connected to ESP32!')
        except Exception as e:
            self.get_logger().error(f'❌ Failed to connect: {e}')
            exit(1)

        # 2. Đăng ký nhận lệnh
        self.subscription = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.listener_callback,
            10)
        self.get_logger().info('Waiting for commands...')

    def listener_callback(self, msg):
        # Lấy dữ liệu từ ROS
        linear_x = msg.linear.x   # Tốc độ tiến/lùi (m/s)
        angular_z = msg.angular.z # Tốc độ xoay (rad/s)
        
        # --- TÍNH TOÁN KINEMATICS (Quan trọng) ---
        # Công thức: V_bánh = V_xe +/- (W_xe * khoảng_cách / 2)
        
        # Bánh trái (Left)
        vel_left = linear_x - (angular_z * WHEEL_DIST / 2.0)
        
        # Bánh phải (Right)
        vel_right = linear_x + (angular_z * WHEEL_DIST / 2.0)

        # --- GỬI XUỐNG ESP32 ---
        # Format lệnh: "v <left> <right>\n"
        command = f"v {vel_left:.3f} {vel_right:.3f}\n"
        
        try:
            self.ser.write(command.encode('utf-8'))
            # Debug: In ra để kiểm tra
            # self.get_logger().info(f'Sent: {command.strip()}')
        except Exception as e:
            self.get_logger().warn(f"Serial Error: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = SerialBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()