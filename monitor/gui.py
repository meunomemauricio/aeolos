"""GUI for monitoring fan speed."""

import numpy as np
import os
import serial
import sys

from PyQt5 import uic
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtWidgets import QApplication, QMainWindow
from pyqtgraph import mkPen


class FanControllerMainWindow(QMainWindow):
    """Application Main window."""

    UPDATE_PERIOD = 22
    WINDOW_SIZE = 660
    PLOT_RANGE = (0, 2000)

    # TODO: Configure port and baud via GUI
    PORT = "/dev/ttyUSB0"
    BAUD_RATE = 115200
    TIMEOUT = 1.0

    def __init__(self, app):
        super(FanControllerMainWindow, self).__init__()

        self.app = app
        self.serial = serial.Serial(
            port=None,
            baudrate=self.BAUD_RATE,
            timeout=self.TIMEOUT,
        )
        # Not defined via constructor so we can open connection later
        self.serial.port = self.PORT

        # Load UI and Set it up
        ui_file = os.path.join(os.path.dirname(__file__), "main_window.ui")
        uic.loadUi(ui_file, self)
        self._initial_button_status()
        self._connect_signals()
        self._set_timer()
        self._setup_plot()

    def _initial_button_status(self):
        """Disable/Enable based on initial status."""
        self.connectButton.setEnabled(True)
        self.disconnectButton.setEnabled(False)

    def _connect_signals(self):
        """Connect the signals to the UI elements."""
        self.actionQuit.triggered.connect(self.close)

        self.connectButton.clicked.connect(self.connect)
        self.disconnectButton.clicked.connect(self.disconnect)

        self.app.aboutToQuit.connect(self.disconnect)

    def _set_timer(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        # TODO: Start/Stop timer with connect/disconnect
        self.timer.start(self.UPDATE_PERIOD)

    def _setup_plot(self):
        self.previous_line = None

        self.speed_data = np.full(self.WINDOW_SIZE, np.nan)
        self.setpoint_data = np.full(self.WINDOW_SIZE, np.nan)

        self.plot_item = self.graphicsView.getPlotItem()
        self.plot_item.setYRange(*self.PLOT_RANGE)
        self.plot_item.setMouseEnabled(False, False)

        self.speed_curve = self.graphicsView.plot(self.speed_data)
        self.setpoint_curve = self.graphicsView.plot(self.setpoint_data)

    def _toggle_buttons(self, connected):
        """Toggle UI buttons in accordance with Connection status."""
        self.connectButton.setEnabled(not connected)
        self.disconnectButton.setEnabled(connected)

    def connect(self):
        """Connect and Sync with the Controller."""
        self.serial.open()
        self._toggle_buttons(connected=True)

    def disconnect(self):
        """Disconnect from the controller."""
        self.serial.close()
        self._toggle_buttons(connected=False)

    def _add_data_points(self, speed: float, setpoint: float) -> None:
        """Add new data points to graph, making sure to rotate data as necessary."""
        self.speed_data[:-1] = self.speed_data[1:]
        self.speed_data[-1] = speed
        self.speed_curve.setData(self.speed_data, connect="finite")

        self.setpoint_data[:-1] = self.setpoint_data[1:]
        self.setpoint_data[-1] = setpoint
        self.setpoint_curve.setData(
            self.setpoint_data,
            connect="finite",
            pen=mkPen("y", style=Qt.DashLine),
        )

    def _update_labels(self, speed: float, setpoint: float) -> None:
        """Display the current Speed and Setpoint in their respective labels."""
        self.rpmLabel.setText(speed.decode("utf-8"))
        self.spLabel.setText(setpoint.decode("utf-8"))

    # TODO: Move to another thread?
    def update_data(self):
        if not self.serial.is_open:
            return

        while self.serial.in_waiting:
            line = self.serial.readline()
            speed, setpoint = line.strip().split(b',', maxsplit=1)
            self._add_data_points(speed=float(speed), setpoint=float(setpoint))
            self._update_labels(speed=speed, setpoint=setpoint)


def run_gui():
    app = QApplication(sys.argv)
    window = FanControllerMainWindow(app)
    window.show()
    sys.exit(app.exec_())
