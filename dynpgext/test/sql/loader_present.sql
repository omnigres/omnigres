-- There should be no loader
SHOW dynpgext.loader_present;
-- And dynpgext should indicate the same
SELECT loader_present();