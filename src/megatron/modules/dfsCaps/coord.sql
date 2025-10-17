DROP TABLE IF EXISTS `tbl_coord`;
CREATE TABLE `tbl_coord` (
  `id` int(20) unsigned NOT NULL auto_increment,
    `lat` double,
    `lon` double,
    `r1` double,
    `r2` double,
    `r3` double,
                    PRIMARY KEY  (`id`),
                      KEY (`r1`),
                      KEY (`r2`),
                      KEY (`r3`)
                            ) ENGINE=MyISAM;
                            