Feature: The calendar has a configuration page that displays and changes the
         calendar along with its specification.

    Scenario: The default specification is almost empty.
       Given we are on the configuration page
        When we write "https://localhost:12345/example.ics" into "calendar-url-input-0"
        Then "url" is specified as "https://localhost:12345/example.ics"

    Scenario: We can specify two URLs.
       Given we are on the configuration page
        When we write "https://localhost:12345/example.ics" into "calendar-url-input-0"
         And we write "https://localhost/example.ics" into "calendar-url-input-1"
        Then "url" is specified as ["https://localhost:12345/example.ics","https://localhost/example.ics"]

    Scenario: We choose a title for our calendar.
       Given we are on the configuration page
        When we write "My Family Calendar" into "calendar-title"
        Then "title" is specified as "My Family Calendar"

    Scenario: We choose the date which the calendar displays
       Given we are on the configuration page
        When we write the date 14/02/2024 into "starting-date"
        Then "date" is specified as "2024-02-14"

    Scenario: We choose the start and end hour
       Given we are on the configuration page
        When we write "6" into "starting-hour"
         And we write "19" into "ending-hour"
        Then "ending_hour" is specified as "19"
         And "starting_hour" is specified as "6"

    Scenario: We choose a time zone to display
       Given we are on the configuration page
        When we choose "Europe/London" in "select-timezone"
        Then "timezone" is specified as "Europe/London"

    Scenario: We choose the tab to display
       Given we are on the configuration page
        When we choose "Week" in "select-tab"
        Then "tab" is specified as "week"
        When we choose "Day" in "select-tab"
        Then "tab" is specified as "day"
        When we choose "Agenda" in "select-tab"
        Then "tab" is specified as "agenda"

    Scenario: We choose the loader
       Given we are on the configuration page
        When we choose "no loader" in "select-loader"
        Then "loader" is specified as ""

    Scenario: We choose the days of the week
       Given we are on the configuration page
        When we choose "Sunday - Saturday" in "select-start-of-week"
        Then "start_of_week" is specified as "su"
        When we choose "Monday - Friday" in "select-start-of-week"
        Then "start_of_week" is specified as "work"

    Scenario: We choose the calendar tabs
       Given we are on the configuration page
        When we click on the span "Month"
        Then "tabs" is specified as ["week","day"]
        When we click on the span "Week"
        Then "tabs" is specified as ["day"]
        When we click on the span "Day"
        Then "tabs" is specified as []
        When we click on the span "Agenda"
        Then "tabs" is specified as ["agenda"]

    Scenario: We choose which controls are visible
       Given we are on the configuration page
        When we click on the span "Date"
        Then "controls" is specified as ["next","previous","today"]
        When we click on the span "Previous"
        Then "controls" is specified as ["next","today"]
        When we click on the span "Today"
        Then "controls" is specified as ["next"]
        When we click on the span "Next"
         And we click on the span "Date"
        Then "controls" is specified as ["date"]

    Scenario: We choose the design
       Given we are on the configuration page
        When we choose "Flat" in "select-skin"
        Then "skin" is specified as "dhtmlxscheduler_flat.css"

    Scenario: We choose to divide the hours
       Given we are on the configuration page
        When we click on the span "10 minutes"
        Then "hour_division" is specified as "6"
        When we click on the span "15 minutes"
        Then "hour_division" is specified as "4"
        When we click on the span "30 minutes"
        Then "hour_division" is specified as "2"
        When we click on the span "1 hour"
        Then "hour_division" is not specified

    Scenario: We choose the language of the calendar
       Given we are on the configuration page
        When we choose "Cymraeg (cy)" in "select-language"
        Then "language" is specified as "cy"

    Scenario Outline: Checkboxes can be checked
       Given we are on the configuration page
        Then "<id>" is not specified
        When we click on the span "<name>"
        Then "<id>" is specified as true

      Examples:
        | name      | id                           |
        | tentative | style-event-status-tentative |
        | confirmed | style-event-status-confirmed |
        | cancelled | style-event-status-cancelled |
