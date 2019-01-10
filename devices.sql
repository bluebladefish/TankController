-- phpMyAdmin SQL Dump
-- version 4.2.12deb2+deb8u2
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Jan 09, 2019 at 06:21 PM
-- Server version: 5.5.59-0+deb8u1
-- PHP Version: 5.6.33-0+deb8u1

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `devices`
--

-- --------------------------------------------------------

--
-- Table structure for table `esp1_log`
--

CREATE TABLE IF NOT EXISTS `esp1_log` (
  `stamp` datetime DEFAULT NULL,
  `varname` tinytext,
  `val` tinytext,
`line_num` int(11) NOT NULL
) ENGINE=InnoDB AUTO_INCREMENT=327 DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `esp2_log`
--

CREATE TABLE IF NOT EXISTS `esp2_log` (
  `stamp` datetime DEFAULT NULL,
  `varname` tinytext,
  `val` tinytext,
`line_num` int(11) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `ESPcont1_conf`
--

CREATE TABLE IF NOT EXISTS `ESPcont1_conf` (
  `varname` varchar(20) NOT NULL,
  `val1` int(3) NOT NULL,
  `val2` int(3) DEFAULT NULL,
  `val3` int(3) DEFAULT NULL,
  `val4` int(3) DEFAULT NULL,
  `stamp` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='ESP controller 1 config';

--
-- Dumping data for table `ESPcont1_conf`
--

INSERT INTO `ESPcont1_conf` (`varname`, `val1`, `val2`, `val3`, `val4`, `stamp`) VALUES
('LtimeOFF', 15, 22, 0, 0, '2018-12-29 03:02:01'),
('LtimeON', 30, 9, 0, 0, '2019-01-10 00:03:51'),
('tempdelta', 1, 0, 0, 0, '2018-12-03 04:57:43'),
('tempgoal', 77, 0, 0, 0, '2018-12-29 02:38:09');

-- --------------------------------------------------------

--
-- Table structure for table `ESPcont2_conf`
--

CREATE TABLE IF NOT EXISTS `ESPcont2_conf` (
  `varname` varchar(20) NOT NULL,
  `val1` int(3) NOT NULL,
  `val2` int(3) DEFAULT NULL,
  `val3` int(3) DEFAULT NULL,
  `val4` int(3) DEFAULT NULL,
  `stamp` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=latin1 COMMENT='ESP controller 1 config';

--
-- Dumping data for table `ESPcont2_conf`
--

INSERT INTO `ESPcont2_conf` (`varname`, `val1`, `val2`, `val3`, `val4`, `stamp`) VALUES
('LtimeOFF', 45, 22, 0, 0, '2018-11-25 22:24:00'),
('LtimeON', 30, 10, 0, 0, '2018-11-25 22:24:30'),
('tempdelta', 1, 0, 0, 0, '2018-11-25 22:30:30'),
('tempgoal', 76, 0, 0, 0, '2018-11-25 22:30:00');

-- --------------------------------------------------------

--
-- Table structure for table `recent`
--

CREATE TABLE IF NOT EXISTS `recent` (
  `varname` varchar(28) NOT NULL,
  `val` varchar(28) NOT NULL,
  `stamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `log_table` varchar(24) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

--
-- Dumping data for table `recent`
--

INSERT INTO `recent` (`varname`, `val`, `stamp`, `log_table`) VALUES
('esp1_heater', 'ON', '2019-01-09 23:57:31', 'esp1_log'),
('esp1_heater_interrupt', 'end', '2019-01-09 23:54:01', 'esp1_log'),
('esp1_lights', 'ON', '2019-01-09 23:51:12', 'esp1_log'),
('esp1_light_interrupt', 'end', '2019-01-09 23:51:22', 'esp1_log'),
('esp1_temp', '69.80', '2019-01-10 00:04:31', 'esp1_log'),
('esp2_lights', 'OFF', '2018-11-25 21:55:33', 'esp2_log');

--
-- Triggers `recent`
--
DELIMITER //
CREATE TRIGGER `on_update_recent` AFTER UPDATE ON `recent`
 FOR EACH ROW BEGIN
	IF NEW.log_table = "esp1_log" THEN
    	INSERT INTO esp1_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
    ELSEIF NEW.log_table = "esp2_log" THEN
    	INSERT INTO esp2_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
    ELSEIF NEW.log_table = "esp3_log" THEN
    	INSERT INTO esp3_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
    ELSEIF NEW.log_table = "esp4_log" THEN
    	INSERT INTO esp4_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
    ELSEIF NEW.log_table = "esp5_log" THEN
    	INSERT INTO esp5_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
    ELSEIF NEW.log_table = "esp6_log" THEN
    	INSERT INTO esp6_log (varname, stamp, val) VALUES (NEW.varname, NEW.stamp, NEW.val);
	END IF;
END
//
DELIMITER ;

--
-- Indexes for dumped tables
--

--
-- Indexes for table `esp1_log`
--
ALTER TABLE `esp1_log`
 ADD PRIMARY KEY (`line_num`);

--
-- Indexes for table `esp2_log`
--
ALTER TABLE `esp2_log`
 ADD PRIMARY KEY (`line_num`);

--
-- Indexes for table `ESPcont1_conf`
--
ALTER TABLE `ESPcont1_conf`
 ADD PRIMARY KEY (`varname`);

--
-- Indexes for table `ESPcont2_conf`
--
ALTER TABLE `ESPcont2_conf`
 ADD PRIMARY KEY (`varname`);

--
-- Indexes for table `recent`
--
ALTER TABLE `recent`
 ADD PRIMARY KEY (`varname`);

--
-- AUTO_INCREMENT for dumped tables
--

--
-- AUTO_INCREMENT for table `esp1_log`
--
ALTER TABLE `esp1_log`
MODIFY `line_num` int(11) NOT NULL AUTO_INCREMENT,AUTO_INCREMENT=327;
--
-- AUTO_INCREMENT for table `esp2_log`
--
ALTER TABLE `esp2_log`
MODIFY `line_num` int(11) NOT NULL AUTO_INCREMENT;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
