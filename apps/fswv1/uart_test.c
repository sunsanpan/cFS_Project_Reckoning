/*
 * Simple UART Test Program
 * Compile: gcc -o uart_test uart_test.c
 * Run: ./uart_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#define UART_DEVICE "/dev/ttyAMA0"
#define UART_BAUDRATE B115200

int main() {
    int uart_fd;
    struct termios tty;
    char buffer[256];
    int buffer_pos = 0;
    ssize_t bytes_read;
    char byte;
    int packet_count = 0;
    
    printf("===========================================\n");
    printf("UART Test Program\n");
    printf("===========================================\n");
    printf("Opening %s at 115200 baud...\n", UART_DEVICE);
    
    // Open UART
    uart_fd = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uart_fd < 0) {
        printf("ERROR: Cannot open %s: %s\n", UART_DEVICE, strerror(errno));
        printf("\nTry:\n");
        printf("  sudo usermod -a -G dialout $USER\n");
        printf("  (then log out and back in)\n");
        return 1;
    }
    
    printf("✓ UART opened successfully!\n\n");
    
    // Get current settings
    if (tcgetattr(uart_fd, &tty) != 0) {
        printf("ERROR: Cannot get UART attributes\n");
        close(uart_fd);
        return 1;
    }
    
    // Configure UART
    cfsetospeed(&tty, UART_BAUDRATE);
    cfsetispeed(&tty, UART_BAUDRATE);
    
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~020000000000;  // CRTSCTS
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST;
    
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
    // Apply settings
    if (tcsetattr(uart_fd, TCSANOW, &tty) != 0) {
        printf("ERROR: Cannot set UART attributes\n");
        close(uart_fd);
        return 1;
    }
    
    tcflush(uart_fd, TCIOFLUSH);
    
    printf("✓ UART configured at 115200 baud, 8N1\n\n");
    printf("Waiting for data...\n");
    printf("Expected format: $,Ax,Ay,Az,Gx,Gy,Gz,T,#\n");
    printf("Press Ctrl+C to exit\n\n");
    printf("-------------------------------------------\n");
    
    // Read loop
    while (1) {
        bytes_read = read(uart_fd, &byte, 1);
        
        if (bytes_read > 0) {
            // Print raw byte for debugging
            if (byte >= 32 && byte <= 126) {
                printf("%c", byte);
            } else {
                printf("[0x%02X]", (unsigned char)byte);
            }
            fflush(stdout);
            
            // Look for start marker
            if (byte == '$') {
                buffer_pos = 0;
                buffer[buffer_pos++] = byte;
            }
            // Look for end marker
            else if (byte == '#') {
                if (buffer_pos > 0 && buffer_pos < 255) {
                    buffer[buffer_pos++] = byte;
                    buffer[buffer_pos] = '\0';
                    
                    packet_count++;
                    printf("\n✓ Packet %d received: %s\n", packet_count, buffer);
                    
                    // Try to parse
                    float ax, ay, az, gx, gy, gz, temp;
                    int parsed = sscanf(buffer, "$,%f,%f,%f,%f,%f,%f,%f,#",
                                      &ax, &ay, &az, &gx, &gy, &gz, &temp);
                    
                    if (parsed == 7) {
                        printf("  Parsed successfully!\n");
                        printf("  Accel: X=%.2f Y=%.2f Z=%.2f\n", ax, ay, az);
                        printf("  Gyro:  X=%.2f Y=%.2f Z=%.2f\n", gx, gy, gz);
                        printf("  Temp:  %.2f °C\n", temp);
                    } else {
                        printf("  ✗ Parse failed (got %d values, expected 7)\n", parsed);
                    }
                    printf("-------------------------------------------\n");
                    
                    buffer_pos = 0;
                }
            }
            // Accumulate data
            else if (buffer_pos > 0 && buffer_pos < 255) {
                buffer[buffer_pos++] = byte;
            }
        }
        
        usleep(1000);  // Sleep 1ms
    }
    
    close(uart_fd);
    return 0;
}
