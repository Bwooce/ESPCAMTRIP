/**
 * GENERATED FILE - DO NOT EDIT
 * Generated on: 2025-12-02T23:27:44.629149
 * Source: YAML service definitions
 * Services: 122
 */

#include "ntrip_atlas.h"
#include "ntrip_coverage_bitmaps.h"

// Provider name lookup table
static const char* provider_names[] = {
    "Instituto Geográfico Nacional (IGN)",  // Index 0
    "Instituto Geográfico Nacional de Argentina (IGN)",  // Index 1
    "IBGE (Brazilian Institute of Geography and Statistics)",  // Index 2
    "Instituto Brasileiro de Geografia e Estatística (IBGE)",  // Index 3
    "Ministère des Transports du Québec",  // Index 4
    "Rx Networks Inc.",  // Index 5
    "GEOCOM S.A.",  // Index 6
    "Instituto Geográfico Agustín Codazzi (IGAC)",  // Index 7
    "Instituto Geográfico Militar del Ecuador (IGM)",  // Index 8
    "Land Information Council of Jamaica (LICJ)",  // Index 9
    "Instituto Nacional de Estadística y Geografía (INEGI)",  // Index 10
    "Instituto Geográfico Nacional (IGN Peru)",  // Index 11
    "Servicio Geográfico Militar (SGM)",  // Index 12
    "Alabama Department of Transportation (ALDOT)",  // Index 13
    "California Spatial Reference Center (CSRC) / UC San Diego",  // Index 14
    "EarthScope Consortium (formerly UNAVCO)",  // Index 15
    "Florida Department of Transportation (FDOT)",  // Index 16
    "Maine Department of Transportation (MaineDOT)",  // Index 17
    "Massachusetts Department of Transportation (MassDOT)",  // Index 18
    "Michigan Department of Transportation (MDOT)",  // Index 19
    "Gulf Coast GNSS Cooperative / Mississippi DOT",  // Index 20
    "Missouri Department of Transportation (MoDOT)",  // Index 21
    "Ohio Department of Transportation (ODOT)",  // Index 22
    "Arizona Department of Water Resources",  // Index 23
    "AZGPS, LLC",  // Index 24
    "EarthScope Consortium (UNAVCO GAGE Facility)",  // Index 25
    "Florida Department of Transportation",  // Index 26
    "Indiana Department of Transportation",  // Index 27
    "Iowa Department of Transportation",  // Index 28
    "Kentucky Transportation Cabinet",  // Index 29
    "Massachusetts Department of Transportation",  // Index 30
    "Michigan Department of Transportation",  // Index 31
    "Minnesota Department of Transportation",  // Index 32
    "Missouri Department of Transportation",  // Index 33
    "New York State Department of Transportation",  // Index 34
    "North Carolina Department of Environment and Natural Resources",  // Index 35
    "Ohio Department of Transportation",  // Index 36
    "Oregon Department of Transportation",  // Index 37
    "Vermont Agency of Transportation",  // Index 38
    "VRSnow USA (Trimble VRS Network)",  // Index 39
    "Wisconsin Department of Transportation",  // Index 40
    "Geoscience Australia",  // Index 41
    "Aptella (Position Partners)",  // Index 42
    "VRSnow Australia (Trimble VRS Network)",  // Index 43
    "Department of Lands and Survey, Fiji",  // Index 44
    "Lands Department, Hong Kong",  // Index 45
    "Survey of India (CORS Network)",  // Index 46
    "Geospatial Information Agency (BIG)",  // Index 47
    "Badan Informasi Geospasial (BIG) - Indonesian Geospatial Information Agency",  // Index 48
    "Geospatial Information Authority of Japan (GSI)",  // Index 49
    "Department of Survey and Mapping Malaysia (JUPEM)",  // Index 50
    "Land Information New Zealand",  // Index 51
    "Survey of Pakistan",  // Index 52
    "National Mapping and Resource Information Authority (NAMRIA)",  // Index 53
    "Singapore Land Authority",  // Index 54
    "National Geographic Information Institute (NGII)",  // Index 55
    "National Land Surveying and Mapping Center (NLSC)",  // Index 56
    "Royal Thai Survey Department",  // Index 57
    "Ministry of Natural Resources and Environment (MONRE)",  // Index 58
    "State Authority for Geospatial Information (ASIG)",  // Index 59
    "Federal Office of Metrology and Surveying (BEV)",  // Index 60
    "EPOSA (Echtzeit Positionierung Austria)",  // Index 61
    "Flemish Government",  // Index 62
    "Service Public de Wallonie",  // Index 63
    "Royal Observatory of Belgium (ROB)",  // Index 64
    "Hexagon Geospatial (HxGN SmartNet Belgium)",  // Index 65
    "Bundesamt für Kartographie und Geodäsie (BKG)",  // Index 66
    "Department of Land Surveys, Ministry of Interior, Republic of Cyprus",  // Index 67
    "Czech Office for Surveying, Mapping and Cadastre (CUZK)",  // Index 68
    "Geoteam A/S (GPSnet Denmark)",  // Index 69
    "Danish Agency for Data Supply and Efficiency (SDFE)",  // Index 70
    "National Land Survey of Finland (NLS)",  // Index 71
    "Centipède RTK Community Network",  // Index 72
    "National Institute of Geographic and Forest Information (IGN)",  // Index 73
    "VRSnow France (Trimble VRS Network)",  // Index 74
    "Federal Agency for Cartography and Geodesy (BKG)",  // Index 75
    "VRSnow Germany (Trimble VRS Network)",  // Index 76
    "Hellenic Cadastre (KTIMATOLOGIO S.A.)",  // Index 77
    "Aristotle University of Thessaloniki (AUTH)",  // Index 78
    "JGC Geotechnical Consultants S.A.",  // Index 79
    "METRICA S.A. (HxGN SmartNet Greece)",  // Index 80
    "Tree Company Corporation AEBE (URANUS Network)",  // Index 81
    "Lechner Tudásközpont Nonprofit Kft. (GNSS Service Center)",  // Index 82
    "Iraqi Geospatial Reference System (IGRS)",  // Index 83
    "Survey of Israel (SOI)",  // Index 84
    "Survey of Israel (SOI) - Active Permanent Network",  // Index 85
    "Regione Autonoma Friuli Venezia Giulia",  // Index 86
    "Italian Space Agency (ASI) - Space Geodesy Center Giuseppe Colombo",  // Index 87
    "Measurement Systems Ltd (Muya CORS Network)",  // Index 88
    "Survey of Kenya (SoK)",  // Index 89
    "National Land Service Under the Ministry of Environment",  // Index 90
    "Royal Centre for Remote Sensing (CRTS)",  // Index 91
    "Kadaster & Rijkswaterstaat",  // Index 92
    "Kadaster (Netherlands Cadastre)",  // Index 93
    "Hexagon Geospatial (HxGN SmartNet Netherlands)",  // Index 94
    "Office of the Surveyor General of the Federation (OSGoF)",  // Index 95
    "Norwegian Mapping Authority (Kartverket)",  // Index 96
    "Blinken AS (TopNet Live Norway)",  // Index 97
    "Head Office of Geodesy and Cartography (GUGiK)",  // Index 98
    "Direção-Geral do Território (DGT) - ReNEP",  // Index 99
    "Direção-Geral do Território (DGT)",  // Index 100
    "Qatar Network for Continuously Operating Reference Stations (QCORS)",  // Index 101
    "National Agency for Cadastre and Land Registration (ANCPI)",  // Index 102
    "General Authority for Survey and Geospatial Information (GEOSA)",  // Index 103
    "Geodesy, Cartography and Cadastre Authority (GKU)",  // Index 104
    "Geodetic Institute of Slovenia (GIS)",  // Index 105
    "Council for Geoscience",  // Index 106
    "National Geographic Institute of Spain (IGN)",  // Index 107
    "Lantmäteriet (Swedish Mapping Authority)",  // Index 108
    "Federal Office of Topography (swisstopo)",  // Index 109
    "General Directorate of Land Registration and Cadastre (TKGM)",  // Index 110
    "Dubai Municipality Survey Department",  // Index 111
    "Ordnance Survey",  // Index 112
    "RTKFnet Ltd",  // Index 113
    "EUREF Permanent Network",  // Index 114
    "GEODNET Community Network",  // Index 115
    "Hexagon AB (Leica Geosystems)",  // Index 116
    "International GNSS Service",  // Index 117
    "Onocoy Community Network",  // Index 118
    "Point One Navigation, Inc.",  // Index 119
    "RTK2go Community",  // Index 120
    "Topcon Positioning Systems, Inc.",  // Index 121
};

// Hierarchical Coverage Bitmaps (Brad Fitzpatrick approach)
// Services are assigned to coverage levels based on quality ratings

// argentina_ramsac: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// argentina_ign_ramsac_ntrip: Coverage levels 0b11111
//   Level 0 (continental): Quality 5
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// brazil_rbmc_ip: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// brazil_rbmc_ip_ibge: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// canada_quebec_rtqc: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 2
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// canada_rx_networks_locationio: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// chile_geocom_gnss: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// colombia_igac_magna_sirgas: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// ecuador_igm_regme_ip: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// jamaica_vrs_licj: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// mexico_inegi_cors: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// peru_regpmoc_ign: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 3

// uruguay_regna_rou_sgm: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// usa_alabama_alcors: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_california_crtn: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 2

// usa_earthscope_nota: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 2

// usa_florida_fdot: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_maine_medot: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// massachusetts_macors: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_michigan_mdot: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_mississippi_gcgc: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_missouri_modot: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_ohio_odot: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_arizona_azcors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// usa_arizona_azgps: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 2
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_earthscope_unavco: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 2
//   Level 4 (local): Quality 2

// usa_florida_fprn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_indiana_incors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_iowa_iartn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_kentucky_kycors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_massachusetts_macors: Coverage levels 0b11110
//   Level 1 (regional): Quality 1
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_michigan_mdot_cors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_minnesota_mncors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_missouri_modot_rtn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_new_york_nysnet: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_north_carolina_rtn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_ohio_odot_rtn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_oregon_orgn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// usa_vermont_vector: Coverage levels 0b11110
//   Level 1 (regional): Quality 1
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// usa_vrsnow: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// usa_wisconsin_wiscors: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 1
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// geoscience_australia: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// australia_aptella_alldayrtk: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// australia_vrsnow: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// fiji_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 4

// hongkong_satref: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// india_soi_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// indonesia_inacors: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// indonesia_big_nrtk: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// japan_gsi_geonet: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// malaysia_myrtknet: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// positionz_rt_nz: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 2

// pakistan_karachi_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 2
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 4

// philippines_pagenet: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// singapore_sirent: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 2
//   Level 4 (local): Quality 5

// south_korea_ngii_cors: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// taiwan_e_gnss: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// thailand_rtsd_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// vietnam_vngeonet_monre: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// albania_asig_gnss: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// austria_apos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// austria_eposa: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// belgium_flepos: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// belgium_walcors: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// euref_rob: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// belgium_smartnet_hxgn: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// bkg_euref_ip_professional: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// cyprus_cypos: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// czech_czepos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// denmark_gpsnet: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// denmark_sdfe_tapas: Coverage levels 0b11110
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 5

// finland_finnref: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// france_centipede_rtk: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 5

// france_rgp: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// france_vrsnow: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// euref_bkg: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// germany_vrsnow: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// greece_hepos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// greece_hermes_auth: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// greece_jgc_net: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// greece_smartnet_hxgn: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// greece_uranus_topnet: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// hungary_gnssnet: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// iraq_igrs_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// israel_soi_apn: Coverage levels 0b11110
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// israel_soi_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// italy_friuli_venezia_giulia: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// italy_asi_geodaf: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// kenya_muya_cors: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// kenya_survey_kenref: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// lithuania_litpos_nls: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// morocco_crts_gnss: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 2
//   Level 3 (state): Quality 2
//   Level 4 (local): Quality 2

// netherlands_netpos: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// netherlands_kadaster: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// netherlands_smartnet_hxgn: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// nigeria_nignet_osgof: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 2
//   Level 4 (local): Quality 2

// norway_satref: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// norway_topnet_live: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// poland_asg_eupos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// portugal_renep: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// portugal_renep_dgt: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// qatar_qcors: Coverage levels 0b11111
//   Level 0 (continental): Quality 1
//   Level 1 (regional): Quality 2
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// romania_rompos_ancpi: Coverage levels 0b11111
//   Level 0 (continental): Quality 3
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// saudi_arabia_ksacors: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// slovakia_skpos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// slovenia_signal_gis: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// southafrica_trignet: Coverage levels 0b11110
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// spain_ergnss: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// sweden_swepos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// switzerland_swipos: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// turkey_tusaga_aktif: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// uae_dubai_vrs: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// uk_osnet_government: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 5

// uk_rtkfnet: Coverage levels 0b11111
//   Level 0 (continental): Quality 2
//   Level 1 (regional): Quality 3
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 4

// euref_ip: Coverage levels 0b11110
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// geodnet_rtk: Coverage levels 0b11111
//   Level 0 (continental): Quality 5
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 3

// hxgn_smartnet: Coverage levels 0b11111
//   Level 0 (continental): Quality 5
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 5
//   Level 3 (state): Quality 5
//   Level 4 (local): Quality 4

// igs_real_time: Coverage levels 0b11111
//   Level 0 (continental): Quality 5
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 2

// onocoy: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 3

// pointone_polaris: Coverage levels 0b11111
//   Level 0 (continental): Quality 5
//   Level 1 (regional): Quality 5
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// rtk2go: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 3
//   Level 3 (state): Quality 3
//   Level 4 (local): Quality 2

// topcon_topnet_live: Coverage levels 0b11111
//   Level 0 (continental): Quality 4
//   Level 1 (regional): Quality 4
//   Level 2 (national): Quality 4
//   Level 3 (state): Quality 4
//   Level 4 (local): Quality 3

// Generated service database
static const ntrip_service_compact_t generated_services[] = {
    // argentina_ramsac - Instituto Geográfico Nacional (IGN)
    {
        .hostname = "ramsac.ign.gob.ar",
        .port = 2101,
        .flags = NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = -5500,
        .lat_max_deg100 = -2170,
        .lon_min_deg100 = -7359,
        .lon_max_deg100 = -5360,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 0,
        .network_type = 1,
        .quality_rating = 4
    },
    // argentina_ign_ramsac_ntrip - Instituto Geográfico Nacional de Argentina (IGN)
    {
        .hostname = "ntrip.ign.gob.ar",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -5500,
        .lat_max_deg100 = -2180,
        .lon_min_deg100 = -7359,
        .lon_max_deg100 = -5360,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 1,
        .network_type = 1,
        .quality_rating = 5
    },
    // brazil_rbmc_ip - IBGE (Brazilian Institute of Geography and Statistics)
    {
        .hostname = "rbmc.ibge.gov.br",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -3370,
        .lat_max_deg100 = 530,
        .lon_min_deg100 = -7390,
        .lon_max_deg100 = -3500,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 2,
        .network_type = 1,
        .quality_rating = 4
    },
    // brazil_rbmc_ip_ibge - Instituto Brasileiro de Geografia e Estatística (IBGE)
    {
        .hostname = "170.84.40.52",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -3379,
        .lat_max_deg100 = 530,
        .lon_min_deg100 = -7390,
        .lon_max_deg100 = -2880,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 3,
        .network_type = 1,
        .quality_rating = 5
    },
    // canada_quebec_rtqc - Ministère des Transports du Québec
    {
        .hostname = "rtqc.mern.gouv.qc.ca",
        .port = 2101,
        .flags = NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = 4500,
        .lat_max_deg100 = 6260,
        .lon_min_deg100 = -7980,
        .lon_max_deg100 = -5710,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 4,
        .network_type = 1,
        .quality_rating = 5
    },
    // canada_rx_networks_locationio - Rx Networks Inc.
    {
        .hostname = "contact-sales.rxnetworks.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4170,
        .lat_max_deg100 = 8310,
        .lon_min_deg100 = -14100,
        .lon_max_deg100 = -5260,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 5,
        .network_type = 2,
        .quality_rating = 5
    },
    // chile_geocom_gnss - GEOCOM S.A.
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = -5600,
        .lat_max_deg100 = -1750,
        .lon_min_deg100 = -10950,
        .lon_max_deg100 = -6600,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 6,
        .network_type = 2,
        .quality_rating = 4
    },
    // colombia_igac_magna_sirgas - Instituto Geográfico Agustín Codazzi (IGAC)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -420,
        .lat_max_deg100 = 1340,
        .lon_min_deg100 = -8200,
        .lon_max_deg100 = -6690,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 7,
        .network_type = 1,
        .quality_rating = 5
    },
    // ecuador_igm_regme_ip - Instituto Geográfico Militar del Ecuador (IGM)
    {
        .hostname = "ntrip.igm.gob.ec",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -500,
        .lat_max_deg100 = 150,
        .lon_min_deg100 = -9200,
        .lon_max_deg100 = -7500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 8,
        .network_type = 1,
        .quality_rating = 5
    },
    // jamaica_vrs_licj - Land Information Council of Jamaica (LICJ)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 1770,
        .lat_max_deg100 = 1860,
        .lon_min_deg100 = -7840,
        .lon_max_deg100 = -7620,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 9,
        .network_type = 1,
        .quality_rating = 4
    },
    // mexico_inegi_cors - Instituto Nacional de Estadística y Geografía (INEGI)
    {
        .hostname = "rgna.inegi.org.mx",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 1450,
        .lat_max_deg100 = 3270,
        .lon_min_deg100 = -11840,
        .lon_max_deg100 = -8670,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 10,
        .network_type = 1,
        .quality_rating = 5
    },
    // peru_regpmoc_ign - Instituto Geográfico Nacional (IGN Peru)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -1839,
        .lat_max_deg100 = 0,
        .lon_min_deg100 = -8130,
        .lon_max_deg100 = -6870,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 11,
        .network_type = 1,
        .quality_rating = 4
    },
    // uruguay_regna_rou_sgm - Servicio Geográfico Militar (SGM)
    {
        .hostname = "200.40.69.58",
        .port = 8081,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -3500,
        .lat_max_deg100 = -3010,
        .lon_min_deg100 = -5840,
        .lon_max_deg100 = -5310,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 12,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_alabama_alcors - Alabama Department of Transportation (ALDOT)
    {
        .hostname = "alcors.dot.state.al.us",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3020,
        .lat_max_deg100 = 3500,
        .lon_min_deg100 = -8850,
        .lon_max_deg100 = -8490,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 13,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_california_crtn - California Spatial Reference Center (CSRC) / UC San Diego
    {
        .hostname = "sopac-csrc.ucsd.edu",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3250,
        .lat_max_deg100 = 4200,
        .lon_min_deg100 = -12450,
        .lon_max_deg100 = -11400,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 14,
        .network_type = 3,
        .quality_rating = 4
    },
    // usa_earthscope_nota - EarthScope Consortium (formerly UNAVCO)
    {
        .hostname = "ntrip.earthscope.org",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 1500,
        .lat_max_deg100 = 7200,
        .lon_min_deg100 = -17000,
        .lon_max_deg100 = -5000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 15,
        .network_type = 3,
        .quality_rating = 4
    },
    // usa_florida_fdot - Florida Department of Transportation (FDOT)
    {
        .hostname = "fdot.fl.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2450,
        .lat_max_deg100 = 3100,
        .lon_min_deg100 = -8760,
        .lon_max_deg100 = -8000,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 16,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_maine_medot - Maine Department of Transportation (MaineDOT)
    {
        .hostname = "medot.maine.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4300,
        .lat_max_deg100 = 4750,
        .lon_min_deg100 = -7109,
        .lon_max_deg100 = -6690,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 17,
        .network_type = 1,
        .quality_rating = 4
    },
    // massachusetts_macors - Massachusetts Department of Transportation (MassDOT)
    {
        .hostname = "64.28.83.185",
        .port = 10000,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4100,
        .lat_max_deg100 = 4300,
        .lon_min_deg100 = -7350,
        .lon_max_deg100 = -6950,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 18,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_michigan_mdot - Michigan Department of Transportation (MDOT)
    {
        .hostname = "mdot.mi.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4170,
        .lat_max_deg100 = 4830,
        .lon_min_deg100 = -9050,
        .lon_max_deg100 = -8240,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 19,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_mississippi_gcgc - Gulf Coast GNSS Cooperative / Mississippi DOT
    {
        .hostname = "gcgc.ms.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3020,
        .lat_max_deg100 = 3500,
        .lon_min_deg100 = -9170,
        .lon_max_deg100 = -8810,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 20,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_missouri_modot - Missouri Department of Transportation (MoDOT)
    {
        .hostname = "modot.mo.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3600,
        .lat_max_deg100 = 4060,
        .lon_min_deg100 = -9580,
        .lon_max_deg100 = -8910,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 21,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_ohio_odot - Ohio Department of Transportation (ODOT)
    {
        .hostname = "odot.ohio.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3840,
        .lat_max_deg100 = 4200,
        .lon_min_deg100 = -8480,
        .lon_max_deg100 = -8050,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 22,
        .network_type = 1,
        .quality_rating = 4
    },
    // usa_arizona_azcors - Arizona Department of Water Resources
    {
        .hostname = "azcors.azwater.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3130,
        .lat_max_deg100 = 3700,
        .lon_min_deg100 = -11480,
        .lon_max_deg100 = -10900,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 23,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_arizona_azgps - AZGPS, LLC
    {
        .hostname = "azgps.net",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 3130,
        .lat_max_deg100 = 3700,
        .lon_min_deg100 = -12440,
        .lon_max_deg100 = -10900,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 24,
        .network_type = 2,
        .quality_rating = 5
    },
    // usa_earthscope_unavco - EarthScope Consortium (UNAVCO GAGE Facility)
    {
        .hostname = "ntrip.earthscope.org",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 1800,
        .lat_max_deg100 = 7200,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = -6500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 25,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_florida_fprn - Florida Department of Transportation
    {
        .hostname = "48.223.232.215",
        .port = 10000,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2450,
        .lat_max_deg100 = 3100,
        .lon_min_deg100 = -8760,
        .lon_max_deg100 = -8000,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 26,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_indiana_incors - Indiana Department of Transportation
    {
        .hostname = "incors.in.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3779,
        .lat_max_deg100 = 4180,
        .lon_min_deg100 = -8810,
        .lon_max_deg100 = -8480,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 27,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_iowa_iartn - Iowa Department of Transportation
    {
        .hostname = "165.206.203.10",
        .port = 10000,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4040,
        .lat_max_deg100 = 4350,
        .lon_min_deg100 = -9660,
        .lon_max_deg100 = -9010,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 28,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_kentucky_kycors - Kentucky Transportation Cabinet
    {
        .hostname = "kycors.ky.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3650,
        .lat_max_deg100 = 3910,
        .lon_min_deg100 = -8960,
        .lon_max_deg100 = -8190,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 29,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_massachusetts_macors - Massachusetts Department of Transportation
    {
        .hostname = "macors.massdot.state.ma.us",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4120,
        .lat_max_deg100 = 4290,
        .lon_min_deg100 = -7350,
        .lon_max_deg100 = -6990,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 30,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_michigan_mdot_cors - Michigan Department of Transportation
    {
        .hostname = "mdotcors.michigan.gov",
        .port = 10000,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4170,
        .lat_max_deg100 = 4830,
        .lon_min_deg100 = -9040,
        .lon_max_deg100 = -8240,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 31,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_minnesota_mncors - Minnesota Department of Transportation
    {
        .hostname = "mncors.dot.state.mn.us",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4350,
        .lat_max_deg100 = 4940,
        .lon_min_deg100 = -9720,
        .lon_max_deg100 = -8950,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 32,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_missouri_modot_rtn - Missouri Department of Transportation
    {
        .hostname = "gpsweb3.modot.mo.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3600,
        .lat_max_deg100 = 4060,
        .lon_min_deg100 = -9580,
        .lon_max_deg100 = -8910,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 33,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_new_york_nysnet - New York State Department of Transportation
    {
        .hostname = "cors.dot.ny.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4050,
        .lat_max_deg100 = 4500,
        .lon_min_deg100 = -7980,
        .lon_max_deg100 = -7190,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 34,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_north_carolina_rtn - North Carolina Department of Environment and Natural Resources
    {
        .hostname = "rtn.nc.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3379,
        .lat_max_deg100 = 3660,
        .lon_min_deg100 = -8430,
        .lon_max_deg100 = -7550,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 35,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_ohio_odot_rtn - Ohio Department of Transportation
    {
        .hostname = "156.63.133.115",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3840,
        .lat_max_deg100 = 4230,
        .lon_min_deg100 = -8480,
        .lon_max_deg100 = -8050,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 36,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_oregon_orgn - Oregon Department of Transportation
    {
        .hostname = "167.131.0.205",
        .port = 9881,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4200,
        .lat_max_deg100 = 4630,
        .lon_min_deg100 = -12460,
        .lon_max_deg100 = -11650,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 37,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_vermont_vector - Vermont Agency of Transportation
    {
        .hostname = "vector.vtrans.vermont.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4270,
        .lat_max_deg100 = 4500,
        .lon_min_deg100 = -7340,
        .lon_max_deg100 = -7150,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 38,
        .network_type = 1,
        .quality_rating = 5
    },
    // usa_vrsnow - VRSnow USA (Trimble VRS Network)
    {
        .hostname = "www.vrsnow.us",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 2490,
        .lat_max_deg100 = 7140,
        .lon_min_deg100 = -17910,
        .lon_max_deg100 = -6800,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 39,
        .network_type = 2,
        .quality_rating = 4
    },
    // usa_wisconsin_wiscors - Wisconsin Department of Transportation
    {
        .hostname = "wiscorsweb.dot.wi.gov",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4250,
        .lat_max_deg100 = 4730,
        .lon_min_deg100 = -9290,
        .lon_max_deg100 = -8680,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 40,
        .network_type = 1,
        .quality_rating = 5
    },
    // geoscience_australia - Geoscience Australia
    {
        .hostname = "ntrip.data.gnss.ga.gov.au",
        .port = 443,
        .flags = NTRIP_FLAG_SSL | NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -5400,
        .lat_max_deg100 = -900,
        .lon_min_deg100 = 7200,
        .lon_max_deg100 = 16800,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 41,
        .network_type = 1,
        .quality_rating = 4
    },
    // australia_aptella_alldayrtk - Aptella (Position Partners)
    {
        .hostname = "alldayrtk.com.au",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = -4700,
        .lat_max_deg100 = -900,
        .lon_min_deg100 = 11000,
        .lon_max_deg100 = 17900,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 42,
        .network_type = 2,
        .quality_rating = 5
    },
    // australia_vrsnow - VRSnow Australia (Trimble VRS Network)
    {
        .hostname = "vrsnow.com.au",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = -4360,
        .lat_max_deg100 = -1070,
        .lon_min_deg100 = 11330,
        .lon_max_deg100 = 15360,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 43,
        .network_type = 2,
        .quality_rating = 4
    },
    // fiji_cors - Department of Lands and Survey, Fiji
    {
        .hostname = "fiji.cors.ga.gov.au",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -2070,
        .lat_max_deg100 = -1250,
        .lon_min_deg100 = 17700,
        .lon_max_deg100 = -17810,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 44,
        .network_type = 1,
        .quality_rating = 4
    },
    // hongkong_satref - Lands Department, Hong Kong
    {
        .hostname = "ntrip.csrw.gov.hk",
        .port = 2101,
        .flags = NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = 2220,
        .lat_max_deg100 = 2250,
        .lon_min_deg100 = 11400,
        .lon_max_deg100 = 11440,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 45,
        .network_type = 1,
        .quality_rating = 4
    },
    // india_soi_cors - Survey of India (CORS Network)
    {
        .hostname = "cors.surveyofindia.gov.in",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 640,
        .lat_max_deg100 = 3710,
        .lon_min_deg100 = 6800,
        .lon_max_deg100 = 9740,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 46,
        .network_type = 1,
        .quality_rating = 5
    },
    // indonesia_inacors - Geospatial Information Agency (BIG)
    {
        .hostname = "103.22.171.6",
        .port = 2001,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -1100,
        .lat_max_deg100 = 600,
        .lon_min_deg100 = 9500,
        .lon_max_deg100 = 14100,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 47,
        .network_type = 1,
        .quality_rating = 4
    },
    // indonesia_big_nrtk - Badan Informasi Geospasial (BIG) - Indonesian Geospatial Information Agency
    {
        .hostname = "nrtk.big.go.id",
        .port = 2001,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -1100,
        .lat_max_deg100 = 610,
        .lon_min_deg100 = 9500,
        .lon_max_deg100 = 14100,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 48,
        .network_type = 1,
        .quality_rating = 4
    },
    // japan_gsi_geonet - Geospatial Information Authority of Japan (GSI)
    {
        .hostname = "mgr.jcn.or.jp",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2400,
        .lat_max_deg100 = 4600,
        .lon_min_deg100 = 12300,
        .lon_max_deg100 = 14600,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 49,
        .network_type = 1,
        .quality_rating = 5
    },
    // malaysia_myrtknet - Department of Survey and Mapping Malaysia (JUPEM)
    {
        .hostname = "202.75.44.154",
        .port = 8080,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 90,
        .lat_max_deg100 = 740,
        .lon_min_deg100 = 9960,
        .lon_max_deg100 = 11930,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 50,
        .network_type = 1,
        .quality_rating = 4
    },
    // positionz_rt_nz - Land Information New Zealand
    {
        .hostname = "positionz-rt.linz.govt.nz",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -4664,
        .lat_max_deg100 = -3445,
        .lon_min_deg100 = 16651,
        .lon_max_deg100 = 17852,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 51,
        .network_type = 3,
        .quality_rating = 4
    },
    // pakistan_karachi_cors - Survey of Pakistan
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2300,
        .lat_max_deg100 = 2800,
        .lon_min_deg100 = 6600,
        .lon_max_deg100 = 7100,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 52,
        .network_type = 1,
        .quality_rating = 3
    },
    // philippines_pagenet - National Mapping and Resource Information Authority (NAMRIA)
    {
        .hostname = "pagenet.namria.gov.ph",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 420,
        .lat_max_deg100 = 2110,
        .lon_min_deg100 = 11600,
        .lon_max_deg100 = 12700,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 53,
        .network_type = 1,
        .quality_rating = 4
    },
    // singapore_sirent - Singapore Land Authority
    {
        .hostname = "199.184.151.36",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 114,
        .lat_max_deg100 = 147,
        .lon_min_deg100 = 10360,
        .lon_max_deg100 = 10410,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 54,
        .network_type = 1,
        .quality_rating = 5
    },
    // south_korea_ngii_cors - National Geographic Information Institute (NGII)
    {
        .hostname = "ntrip.ngii.go.kr",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3300,
        .lat_max_deg100 = 3860,
        .lon_min_deg100 = 12460,
        .lon_max_deg100 = 13190,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 55,
        .network_type = 1,
        .quality_rating = 5
    },
    // taiwan_e_gnss - National Land Surveying and Mapping Center (NLSC)
    {
        .hostname = "egnss.nlsc.gov.tw",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2190,
        .lat_max_deg100 = 2530,
        .lon_min_deg100 = 11950,
        .lon_max_deg100 = 12200,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 56,
        .network_type = 1,
        .quality_rating = 5
    },
    // thailand_rtsd_cors - Royal Thai Survey Department
    {
        .hostname = "cors.rtsd.mi.th",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 560,
        .lat_max_deg100 = 2050,
        .lon_min_deg100 = 9730,
        .lon_max_deg100 = 10560,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 57,
        .network_type = 1,
        .quality_rating = 4
    },
    // vietnam_vngeonet_monre - Ministry of Natural Resources and Environment (MONRE)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 819,
        .lat_max_deg100 = 2340,
        .lon_min_deg100 = 10210,
        .lon_max_deg100 = 10950,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 58,
        .network_type = 1,
        .quality_rating = 5
    },
    // albania_asig_gnss - State Authority for Geospatial Information (ASIG)
    {
        .hostname = "asig.gov.al",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3960,
        .lat_max_deg100 = 4270,
        .lon_min_deg100 = 1930,
        .lon_max_deg100 = 2110,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 59,
        .network_type = 1,
        .quality_rating = 4
    },
    // austria_apos - Federal Office of Metrology and Surveying (BEV)
    {
        .hostname = "apos.bev.gv.at",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4640,
        .lat_max_deg100 = 4900,
        .lon_min_deg100 = 950,
        .lon_max_deg100 = 1720,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 60,
        .network_type = 1,
        .quality_rating = 5
    },
    // austria_eposa - EPOSA (Echtzeit Positionierung Austria)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4640,
        .lat_max_deg100 = 4900,
        .lon_min_deg100 = 950,
        .lon_max_deg100 = 1720,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 61,
        .network_type = 2,
        .quality_rating = 5
    },
    // belgium_flepos - Flemish Government
    {
        .hostname = "flepos.vlaanderen.be",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5070,
        .lat_max_deg100 = 5150,
        .lon_min_deg100 = 250,
        .lon_max_deg100 = 590,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 62,
        .network_type = 1,
        .quality_rating = 4
    },
    // belgium_walcors - Service Public de Wallonie
    {
        .hostname = "gnss.wallonie.be",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4950,
        .lat_max_deg100 = 5070,
        .lon_min_deg100 = 270,
        .lon_max_deg100 = 640,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 63,
        .network_type = 1,
        .quality_rating = 4
    },
    // euref_rob - Royal Observatory of Belgium (ROB)
    {
        .hostname = "euref-ip.be",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4950,
        .lat_max_deg100 = 5150,
        .lon_min_deg100 = 250,
        .lon_max_deg100 = 650,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 64,
        .network_type = 1,
        .quality_rating = 5
    },
    // belgium_smartnet_hxgn - Hexagon Geospatial (HxGN SmartNet Belgium)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4950,
        .lat_max_deg100 = 5150,
        .lon_min_deg100 = 250,
        .lon_max_deg100 = 640,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 65,
        .network_type = 2,
        .quality_rating = 5
    },
    // bkg_euref_ip_professional - Bundesamt für Kartographie und Geodäsie (BKG)
    {
        .hostname = "euref-ip.net",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3500,
        .lat_max_deg100 = 7100,
        .lon_min_deg100 = -2500,
        .lon_max_deg100 = 4500,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 66,
        .network_type = 1,
        .quality_rating = 4
    },
    // cyprus_cypos - Department of Land Surveys, Ministry of Interior, Republic of Cyprus
    {
        .hostname = "cypos.dls.moi.gov.cy",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3450,
        .lat_max_deg100 = 3570,
        .lon_min_deg100 = 3220,
        .lon_max_deg100 = 3460,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 67,
        .network_type = 1,
        .quality_rating = 5
    },
    // czech_czepos - Czech Office for Surveying, Mapping and Cadastre (CUZK)
    {
        .hostname = "czepos.cuzk.cz",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4850,
        .lat_max_deg100 = 5110,
        .lon_min_deg100 = 1200,
        .lon_max_deg100 = 1889,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 68,
        .network_type = 1,
        .quality_rating = 5
    },
    // denmark_gpsnet - Geoteam A/S (GPSnet Denmark)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 5450,
        .lat_max_deg100 = 5780,
        .lon_min_deg100 = 800,
        .lon_max_deg100 = 1520,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 69,
        .network_type = 2,
        .quality_rating = 5
    },
    // denmark_sdfe_tapas - Danish Agency for Data Supply and Efficiency (SDFE)
    {
        .hostname = "tapas.sdfe.dk",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5450,
        .lat_max_deg100 = 5780,
        .lon_min_deg100 = 800,
        .lon_max_deg100 = 1520,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 70,
        .network_type = 1,
        .quality_rating = 4
    },
    // finland_finnref - National Land Survey of Finland (NLS)
    {
        .hostname = "finpos.nls.fi",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5950,
        .lat_max_deg100 = 7000,
        .lon_min_deg100 = 1900,
        .lon_max_deg100 = 3150,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 71,
        .network_type = 1,
        .quality_rating = 4
    },
    // france_centipede_rtk - Centipède RTK Community Network
    {
        .hostname = "caster.centipede.fr",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = 4130,
        .lat_max_deg100 = 5110,
        .lon_min_deg100 = -530,
        .lon_max_deg100 = 960,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 72,
        .network_type = 3,
        .quality_rating = 4
    },
    // france_rgp - National Institute of Geographic and Forest Information (IGN)
    {
        .hostname = "rgp.ign.fr",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = -2150,
        .lat_max_deg100 = 5150,
        .lon_min_deg100 = -6300,
        .lon_max_deg100 = 1000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 73,
        .network_type = 1,
        .quality_rating = 5
    },
    // france_vrsnow - VRSnow France (Trimble VRS Network)
    {
        .hostname = "vrsnow.fr",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4130,
        .lat_max_deg100 = 5110,
        .lon_min_deg100 = -520,
        .lon_max_deg100 = 960,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 74,
        .network_type = 2,
        .quality_rating = 4
    },
    // euref_bkg - Federal Agency for Cartography and Geodesy (BKG)
    {
        .hostname = "euref-ip.net",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4700,
        .lat_max_deg100 = 5500,
        .lon_min_deg100 = 600,
        .lon_max_deg100 = 1500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 75,
        .network_type = 1,
        .quality_rating = 5
    },
    // germany_vrsnow - VRSnow Germany (Trimble VRS Network)
    {
        .hostname = "vrsnow.de",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4730,
        .lat_max_deg100 = 5510,
        .lon_min_deg100 = 590,
        .lon_max_deg100 = 1500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 76,
        .network_type = 2,
        .quality_rating = 4
    },
    // greece_hepos - Hellenic Cadastre (KTIMATOLOGIO S.A.)
    {
        .hostname = "hepos.gr",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3450,
        .lat_max_deg100 = 4180,
        .lon_min_deg100 = 1930,
        .lon_max_deg100 = 2970,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 77,
        .network_type = 1,
        .quality_rating = 5
    },
    // greece_hermes_auth - Aristotle University of Thessaloniki (AUTH)
    {
        .hostname = "academic.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3900,
        .lat_max_deg100 = 4200,
        .lon_min_deg100 = 2000,
        .lon_max_deg100 = 2650,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 78,
        .network_type = 1,
        .quality_rating = 4
    },
    // greece_jgc_net - JGC Geotechnical Consultants S.A.
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 3450,
        .lat_max_deg100 = 4200,
        .lon_min_deg100 = 1930,
        .lon_max_deg100 = 3500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 79,
        .network_type = 2,
        .quality_rating = 4
    },
    // greece_smartnet_hxgn - METRICA S.A. (HxGN SmartNet Greece)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 3479,
        .lat_max_deg100 = 4180,
        .lon_min_deg100 = 1930,
        .lon_max_deg100 = 2960,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 80,
        .network_type = 2,
        .quality_rating = 4
    },
    // greece_uranus_topnet - Tree Company Corporation AEBE (URANUS Network)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 3450,
        .lat_max_deg100 = 4200,
        .lon_min_deg100 = 1930,
        .lon_max_deg100 = 3500,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 81,
        .network_type = 2,
        .quality_rating = 5
    },
    // hungary_gnssnet - Lechner Tudásközpont Nonprofit Kft. (GNSS Service Center)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4570,
        .lat_max_deg100 = 4860,
        .lon_min_deg100 = 1610,
        .lon_max_deg100 = 2290,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 82,
        .network_type = 1,
        .quality_rating = 5
    },
    // iraq_igrs_cors - Iraqi Geospatial Reference System (IGRS)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2910,
        .lat_max_deg100 = 3740,
        .lon_min_deg100 = 3879,
        .lon_max_deg100 = 4860,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 83,
        .network_type = 1,
        .quality_rating = 4
    },
    // israel_soi_apn - Survey of Israel (SOI)
    {
        .hostname = "apn.mapi.gov.il",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2950,
        .lat_max_deg100 = 3329,
        .lon_min_deg100 = 3420,
        .lon_max_deg100 = 3590,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 84,
        .network_type = 1,
        .quality_rating = 5
    },
    // israel_soi_cors - Survey of Israel (SOI) - Active Permanent Network
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2940,
        .lat_max_deg100 = 3329,
        .lon_min_deg100 = 3420,
        .lon_max_deg100 = 3590,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 85,
        .network_type = 1,
        .quality_rating = 5
    },
    // italy_friuli_venezia_giulia - Regione Autonoma Friuli Venezia Giulia
    {
        .hostname = "gnsscaster.regione.fvg.it",
        .port = 8080,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4550,
        .lat_max_deg100 = 4670,
        .lon_min_deg100 = 1230,
        .lon_max_deg100 = 1390,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 86,
        .network_type = 1,
        .quality_rating = 4
    },
    // italy_asi_geodaf - Italian Space Agency (ASI) - Space Geodesy Center Giuseppe Colombo
    {
        .hostname = "euref-ip.asi.it",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3660,
        .lat_max_deg100 = 4710,
        .lon_min_deg100 = 660,
        .lon_max_deg100 = 1850,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 87,
        .network_type = 1,
        .quality_rating = 5
    },
    // kenya_muya_cors - Measurement Systems Ltd (Muya CORS Network)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = -470,
        .lat_max_deg100 = 500,
        .lon_min_deg100 = 3390,
        .lon_max_deg100 = 4190,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 88,
        .network_type = 2,
        .quality_rating = 4
    },
    // kenya_survey_kenref - Survey of Kenya (SoK)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = -470,
        .lat_max_deg100 = 500,
        .lon_min_deg100 = 3390,
        .lon_max_deg100 = 4190,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 89,
        .network_type = 1,
        .quality_rating = 4
    },
    // lithuania_litpos_nls - National Land Service Under the Ministry of Environment
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5390,
        .lat_max_deg100 = 5650,
        .lon_min_deg100 = 2090,
        .lon_max_deg100 = 2690,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 90,
        .network_type = 1,
        .quality_rating = 5
    },
    // morocco_crts_gnss - Royal Centre for Remote Sensing (CRTS)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2100,
        .lat_max_deg100 = 3600,
        .lon_min_deg100 = -1710,
        .lon_max_deg100 = -100,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 91,
        .network_type = 1,
        .quality_rating = 3
    },
    // netherlands_netpos - Kadaster & Rijkswaterstaat
    {
        .hostname = "ntrip.kadaster.nl",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5050,
        .lat_max_deg100 = 5370,
        .lon_min_deg100 = 300,
        .lon_max_deg100 = 750,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 92,
        .network_type = 1,
        .quality_rating = 4
    },
    // netherlands_kadaster - Kadaster (Netherlands Cadastre)
    {
        .hostname = "ntrip.kadaster.nl",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5070,
        .lat_max_deg100 = 5370,
        .lon_min_deg100 = 330,
        .lon_max_deg100 = 730,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 93,
        .network_type = 1,
        .quality_rating = 5
    },
    // netherlands_smartnet_hxgn - Hexagon Geospatial (HxGN SmartNet Netherlands)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 5070,
        .lat_max_deg100 = 5360,
        .lon_min_deg100 = 320,
        .lon_max_deg100 = 730,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 94,
        .network_type = 2,
        .quality_rating = 5
    },
    // nigeria_nignet_osgof - Office of the Surveyor General of the Federation (OSGoF)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 400,
        .lat_max_deg100 = 1400,
        .lon_min_deg100 = 270,
        .lon_max_deg100 = 1470,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 95,
        .network_type = 1,
        .quality_rating = 3
    },
    // norway_satref - Norwegian Mapping Authority (Kartverket)
    {
        .hostname = "159.162.103.14",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5700,
        .lat_max_deg100 = 8100,
        .lon_min_deg100 = 400,
        .lon_max_deg100 = 3200,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 96,
        .network_type = 1,
        .quality_rating = 5
    },
    // norway_topnet_live - Blinken AS (TopNet Live Norway)
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 5790,
        .lat_max_deg100 = 7120,
        .lon_min_deg100 = 459,
        .lon_max_deg100 = 3130,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 97,
        .network_type = 2,
        .quality_rating = 5
    },
    // poland_asg_eupos - Head Office of Geodesy and Cartography (GUGiK)
    {
        .hostname = "asgeupos.pl",
        .port = 2101,
        .flags = NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = 4900,
        .lat_max_deg100 = 5500,
        .lon_min_deg100 = 1400,
        .lon_max_deg100 = 2450,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 98,
        .network_type = 1,
        .quality_rating = 5
    },
    // portugal_renep - Direção-Geral do Território (DGT) - ReNEP
    {
        .hostname = "renep.dgterritorio.gov.pt",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3240,
        .lat_max_deg100 = 4220,
        .lon_min_deg100 = -3130,
        .lon_max_deg100 = -620,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 99,
        .network_type = 1,
        .quality_rating = 5
    },
    // portugal_renep_dgt - Direção-Geral do Território (DGT)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3240,
        .lat_max_deg100 = 4220,
        .lon_min_deg100 = -3130,
        .lon_max_deg100 = -620,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 100,
        .network_type = 1,
        .quality_rating = 5
    },
    // qatar_qcors - Qatar Network for Continuously Operating Reference Stations (QCORS)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2440,
        .lat_max_deg100 = 2620,
        .lon_min_deg100 = 5070,
        .lon_max_deg100 = 5170,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 101,
        .network_type = 1,
        .quality_rating = 5
    },
    // romania_rompos_ancpi - National Agency for Cadastre and Land Registration (ANCPI)
    {
        .hostname = "ROMPOS-Proxy",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4360,
        .lat_max_deg100 = 4830,
        .lon_min_deg100 = 2020,
        .lon_max_deg100 = 2970,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 102,
        .network_type = 1,
        .quality_rating = 5
    },
    // saudi_arabia_ksacors - General Authority for Survey and Geospatial Information (GEOSA)
    {
        .hostname = "auth-required.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 1560,
        .lat_max_deg100 = 3220,
        .lon_min_deg100 = 3450,
        .lon_max_deg100 = 5570,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 103,
        .network_type = 1,
        .quality_rating = 5
    },
    // slovakia_skpos - Geodesy, Cartography and Cadastre Authority (GKU)
    {
        .hostname = "skpos.gku.sk",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4770,
        .lat_max_deg100 = 4960,
        .lon_min_deg100 = 1680,
        .lon_max_deg100 = 2260,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 104,
        .network_type = 1,
        .quality_rating = 5
    },
    // slovenia_signal_gis - Geodetic Institute of Slovenia (GIS)
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4540,
        .lat_max_deg100 = 4690,
        .lon_min_deg100 = 1340,
        .lon_max_deg100 = 1660,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 105,
        .network_type = 1,
        .quality_rating = 5
    },
    // southafrica_trignet - Council for Geoscience
    {
        .hostname = "trignet.sapos.gov.za",
        .port = 2101,
        .flags = NTRIP_FLAG_FREE_ACCESS,
        .lat_min_deg100 = -4700,
        .lat_max_deg100 = -2200,
        .lon_min_deg100 = 1600,
        .lon_max_deg100 = 3300,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 106,
        .network_type = 1,
        .quality_rating = 4
    },
    // spain_ergnss - National Geographic Institute of Spain (IGN)
    {
        .hostname = "ergnss.ign.es",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2750,
        .lat_max_deg100 = 4380,
        .lon_min_deg100 = -1850,
        .lon_max_deg100 = 500,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 107,
        .network_type = 1,
        .quality_rating = 5
    },
    // sweden_swepos - Lantmäteriet (Swedish Mapping Authority)
    {
        .hostname = "nrtk-swepos.lm.se",
        .port = 80,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 5500,
        .lat_max_deg100 = 6950,
        .lon_min_deg100 = 1050,
        .lon_max_deg100 = 2450,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 108,
        .network_type = 1,
        .quality_rating = 5
    },
    // switzerland_swipos - Federal Office of Topography (swisstopo)
    {
        .hostname = "swipos.swisstopo.ch",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4580,
        .lat_max_deg100 = 4780,
        .lon_min_deg100 = 590,
        .lon_max_deg100 = 1050,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 109,
        .network_type = 1,
        .quality_rating = 5
    },
    // turkey_tusaga_aktif - General Directorate of Land Registration and Cadastre (TKGM)
    {
        .hostname = "212.156.70.42",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3579,
        .lat_max_deg100 = 4210,
        .lon_min_deg100 = 2570,
        .lon_max_deg100 = 4480,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 110,
        .network_type = 1,
        .quality_rating = 5
    },
    // uae_dubai_vrs - Dubai Municipality Survey Department
    {
        .hostname = "register.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 2480,
        .lat_max_deg100 = 2540,
        .lon_min_deg100 = 5490,
        .lon_max_deg100 = 5560,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 111,
        .network_type = 1,
        .quality_rating = 5
    },
    // uk_osnet_government - Ordnance Survey
    {
        .hostname = "os-net.ordnancesurvey.co.uk",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 4980,
        .lat_max_deg100 = 6090,
        .lon_min_deg100 = -869,
        .lon_max_deg100 = 200,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 112,
        .network_type = 1,
        .quality_rating = 5
    },
    // uk_rtkfnet - RTKFnet Ltd
    {
        .hostname = "contact-sales.example.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE,
        .lat_min_deg100 = 4990,
        .lat_max_deg100 = 6090,
        .lon_min_deg100 = -819,
        .lon_max_deg100 = 200,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 113,
        .network_type = 2,
        .quality_rating = 4
    },
    // euref_ip - EUREF Permanent Network
    {
        .hostname = "euref-ip.net",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG,
        .lat_min_deg100 = 3500,
        .lat_max_deg100 = 7100,
        .lon_min_deg100 = -1000,
        .lon_max_deg100 = 4000,
        .coverage_levels = 0b11110,  // 30
        .reserved = 0,
        .provider_index = 114,
        .network_type = 1,
        .quality_rating = 5
    },
    // geodnet_rtk - GEODNET Community Network
    {
        .hostname = "rtk.geodnet.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -9000,
        .lat_max_deg100 = 9000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 115,
        .network_type = 3,
        .quality_rating = 4
    },
    // hxgn_smartnet - Hexagon AB (Leica Geosystems)
    {
        .hostname = "www.smartnetna.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -5000,
        .lat_max_deg100 = 7500,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 116,
        .network_type = 2,
        .quality_rating = 4
    },
    // igs_real_time - International GNSS Service
    {
        .hostname = "products.igs-ip.net",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -9000,
        .lat_max_deg100 = 9000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 117,
        .network_type = 1,
        .quality_rating = 5
    },
    // onocoy - Onocoy Community Network
    {
        .hostname = "onocoy.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -9000,
        .lat_max_deg100 = 9000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 118,
        .network_type = 3,
        .quality_rating = 4
    },
    // pointone_polaris - Point One Navigation, Inc.
    {
        .hostname = "polaris.pointonenav.com",
        .port = 443,
        .flags = NTRIP_FLAG_SSL | NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -5000,
        .lat_max_deg100 = 7000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 119,
        .network_type = 2,
        .quality_rating = 4
    },
    // rtk2go - RTK2go Community
    {
        .hostname = "rtk2go.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -9000,
        .lat_max_deg100 = 9000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 120,
        .network_type = 3,
        .quality_rating = 4
    },
    // topcon_topnet_live - Topcon Positioning Systems, Inc.
    {
        .hostname = "rtk.topnetlive.com",
        .port = 2101,
        .flags = NTRIP_FLAG_AUTH_BASIC | NTRIP_FLAG_REQUIRES_REG | NTRIP_FLAG_PAID_SERVICE | NTRIP_FLAG_GLOBAL_SERVICE,
        .lat_min_deg100 = -4500,
        .lat_max_deg100 = 7000,
        .lon_min_deg100 = -18000,
        .lon_max_deg100 = 18000,
        .coverage_levels = 0b11111,  // 31
        .reserved = 0,
        .provider_index = 121,
        .network_type = 2,
        .quality_rating = 4
    }
};

#define GENERATED_SERVICE_COUNT 122

const ntrip_service_compact_t* get_generated_services(size_t* count) {
    *count = GENERATED_SERVICE_COUNT;
    return generated_services;
}

const char* get_provider_name(uint8_t provider_index) {
    if (provider_index >= 122) return "Unknown";
    return provider_names[provider_index];
}