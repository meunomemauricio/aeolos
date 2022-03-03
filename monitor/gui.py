"""GUI for manually controlling the fan."""

import numpy as np
import os
import sys

from PyQt5 import uic
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtGui import QIntValidator, QValidator, QDoubleValidator
from PyQt5.QtWidgets import QApplication, QMainWindow
from pyqtgraph import mkPen

from api.fan_interface import FanInterface, NotConnectedError, ReadTimeoutError


class FanControllerMainWindow(QMainWindow):
    """Application Main window."""

    MAX_ACTUATOR = 88

    UPDATE_PERIOD = 33
    WINDOW_SIZE = 660
    PLOT_RANGE = (0, 4000)

    MIN_SETPOINT = 750
    MAX_SETPOINT = 3700

    TOOLTIPS = {
        "setpointInput": (
            "Setpoint of the Controller in RPMs (Min: {}, Max: {})".format(
                MIN_SETPOINT, MAX_SETPOINT,
            )
        ),
    }

    def __init__(self, app):
        super(FanControllerMainWindow, self).__init__()

        self.app = app
        self.fan_iface = FanInterface()

        # Load UI and Set it up
        ui_file = os.path.join(os.path.dirname(__file__), "main_window.ui")
        uic.loadUi(ui_file, self)
        self._initial_button_status()
        self._connect_signals()
        self._set_tooltips()
        self._set_validators()
        self._set_timer()
        self._setup_plot()

        self.fan_on = False
        self.setpoint = np.nan

    def _initial_button_status(self):
        """Disable/Enable based on initial status."""
        self.applySetpointButton.setEnabled(False)
        self.connectButton.setEnabled(True)
        self.disconnectButton.setEnabled(False)
        self.offButton.setEnabled(False)
        self.onButton.setEnabled(False)
        self.setParamsButton.setEnabled(False)

    def _connect_signals(self):
        """Connect the signals to the UI elements."""
        self.actionQuit.triggered.connect(self.close)

        self.applySetpointButton.clicked.connect(self.apply_setpoint)
        self.connectButton.clicked.connect(self.connect)
        self.disconnectButton.clicked.connect(self.disconnect)
        self.offButton.clicked.connect(self.turn_fan_off)
        self.onButton.clicked.connect(self.turn_fan_on)
        self.readParamsButton.clicked.connect(self.read_parameters)
        self.setParamsButton.clicked.connect(self.set_parameters)

        self.app.aboutToQuit.connect(self.disconnect)

    def _set_tooltips(self):
        """Set ToolTips to the UI elements."""
        self.setpointInput.setToolTip(self.TOOLTIPS["setpointInput"])

    def _set_validators(self):
        sp_validator = QIntValidator(self.MIN_SETPOINT, self.MAX_SETPOINT)
        self.setpointInput.setValidator(sp_validator)
        self.kpInput.setValidator(QDoubleValidator())
        self.kiInput.setValidator(QDoubleValidator())
        self.kdInput.setValidator(QDoubleValidator())

    def _set_timer(self):
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_data)
        self.timer.start(self.UPDATE_PERIOD)

    def _setup_plot(self):
        self.previous_line = None

        self.speed_data = np.full(self.WINDOW_SIZE, np.nan)
        self.actuator_data = np.full(self.WINDOW_SIZE, np.nan)
        self.setpoint_data = np.full(self.WINDOW_SIZE, np.nan)

        self.plot_item = self.graphicsView.getPlotItem()
        self.plot_item.setYRange(*self.PLOT_RANGE)
        self.plot_item.setMouseEnabled(False, False)
        self.speed_curve = self.graphicsView.plot(self.speed_data)
        self.actuator_curve = self.graphicsView.plot(self.actuator_data)
        self.setpoint_curve = self.graphicsView.plot(self.setpoint_data)

    def _update_params_line_edit(self, kp, ki, kd):
        """Update the values in the Parameters LineEdits."""
        self.kpInput.setText("{:0.4f}".format(kp))
        self.kiInput.setText("{:0.4f}".format(ki))
        self.kdInput.setText("{:0.4f}".format(kd))

    def _toggle_buttons(self, connected):
        """Toggle UI buttons in accordance with Connection status."""
        self.applySetpointButton.setEnabled(connected)
        self.connectButton.setEnabled(not connected)
        self.disconnectButton.setEnabled(connected)
        self.setParamsButton.setEnabled(connected)
        self.readParamsButton.setEnabled(connected)

        self.offButton.setEnabled(connected and self.fan_on)
        self.onButton.setEnabled(connected and not self.fan_on)

    def _valid_inputs(self, input_fields):
        """Validate a list of input field.

        Colors are applied to the fields depending on the validation result.

        :param input_fields: List of field objects to be validated.
        :return: True if all inputs are valid, False otherwise
        """
        inputs_are_valid = []
        for field in input_fields:
            state = field.validator().validate(field.text(), 0)[0]
            if state == QValidator.Acceptable:
                inputs_are_valid.append(True)
                color = "#c4df9b"  # Green
            elif state == QValidator.Intermediate:
                inputs_are_valid.append(False)
                color = "#fff79a"  # Yellow
            else:
                inputs_are_valid.append(False)
                color = "#f6989d"  # Red

            # Change widget background to indicate validator status
            style = "QLineEdit {{ background-color: {} }}".format(color)
            field.setStyleSheet(style)

        return all(inputs_are_valid)

    def _update_setpoint(self):
        """Read the setpoint from the controller and update values/input"""
        self.setpoint = self.fan_iface.read_setpoint()
        self.setpointInput.setText("{}".format(self.setpoint))

    def connect(self):
        """Connect and Sync with the Controller."""
        self.fan_iface.connect("/dev/ttyUSB0", 115200)
        self.read_parameters()
        self.fan_on = self.fan_iface.read_status()
        self._update_setpoint()
        self._toggle_buttons(connected=True)

    def disconnect(self):
        """Disconnect from the controller."""
        self.fan_iface.disconnect()
        self.setpoint = np.nan
        self._toggle_buttons(connected=False)

    def apply_setpoint(self):
        """Send the Setpoint Value to the controller."""
        input_field = self.setpointInput
        state = input_field.validator().validate(input_field.text(), 0)[0]
        if state == QValidator.Acceptable:
            self.setpoint = int(self.setpointInput.text())
            try:
                self.fan_iface.update_setpoint(self.setpoint)
            except NotConnectedError:
                color = "#f6989d"  # Red
            else:
                color = "#c4df9b"  # Green
        else:
            color = "#fff79a"  # Yellow

        # Change widget background to indicate validator status
        style = "QLineEdit {{ background-color: {} }}".format(color)
        input_field.setStyleSheet(style)

    def _update_labels(self, rpm_value, actuator):
        speed_label = "---"
        if rpm_value:
            speed_label = "{:.0f} RPM".format(rpm_value)

        actuator_label = "---"
        if actuator is not np.nan:
            actuator_label = "{curr}/{max} ({pctg:.2%})".format(
                curr=actuator,
                max=self.MAX_ACTUATOR,
                pctg=(actuator / float(self.MAX_ACTUATOR)),
            )

        self.speedLabel.setText(speed_label)
        self.actuatorLabel.setText(actuator_label)

    # TODO: Move to another thread
    def update_data(self):
        rpm_value = np.nan
        actuator = np.nan
        if self.fan_iface.connected:
            try:
                rpm_value = self.fan_iface.read_sensor()[1]  # Ignore timestamp
                actuator = self.fan_iface.read_actuator()
            except ReadTimeoutError:
                pass

        self._update_labels(rpm_value, actuator)

        self.speed_data[:-1] = self.speed_data[1:]
        self.speed_data[-1] = rpm_value
        self.speed_curve.setData(self.speed_data, connect="finite")

        self.actuator_data[:-1] = self.actuator_data[1:]
        self.actuator_data[-1] = actuator
        self.actuator_curve.setData(self.actuator_data, connect="finite")

        self.setpoint_data[:-1] = self.setpoint_data[1:]
        self.setpoint_data[-1] = self.setpoint
        self.setpoint_curve.setData(
            self.setpoint_data,
            connect="finite",
            pen=mkPen("y", style=Qt.DashLine),
        )

    # TODO: Move to another thread???
    def read_parameters(self):
        """Read the controller parameters and update Line Edits."""
        while True:  # TODO: Implement some timeout
            try:
                kp, ki, kd = self.fan_iface.read_parameters()
            except ReadTimeoutError:
                print("Timeout")
            else:
                self._update_params_line_edit(kp, ki, kd)
                break

    def set_parameters(self):
        """Send the Parameters values in the UI to the controller."""
        if not self._valid_inputs([self.kpInput, self.kiInput, self.kdInput]):
            return

        self.fan_iface.update_parameters(
            kp=float(self.kpInput.text()),
            ki=float(self.kiInput.text()),
            kd=float(self.kdInput.text()),
        )

    def turn_fan_off(self):
        self.setpoint = np.nan
        self.fan_iface.turn_off()
        self.fan_on = False
        self.offButton.setEnabled(False)
        self.onButton.setEnabled(True)

    def turn_fan_on(self):
        self._update_setpoint()
        self.fan_iface.turn_on()
        self.fan_on = True
        self.offButton.setEnabled(True)
        self.onButton.setEnabled(False)


def run_gui():
    app = QApplication(sys.argv)
    window = FanControllerMainWindow(app)
    window.show()
    sys.exit(app.exec_())
