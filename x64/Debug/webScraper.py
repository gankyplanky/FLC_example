from asyncio.windows_events import NULL
from types import NoneType
from bs4 import BeautifulSoup
from selenium import webdriver
from selenium.webdriver.common.by import By
from selenium.webdriver.support.ui import WebDriverWait
from selenium.common.exceptions import NoSuchElementException
import requests

chrome_options = webdriver.ChromeOptions()
chrome_options.add_argument("--disable-notifications")

driver = webdriver.Chrome(chrome_options=chrome_options)
driver.minimize_window()
driver.get("https://www.mozzartbet.com/sr#/betting")
wait = WebDriverWait(driver, 1)
originalWindow = driver.current_window_handle



try:
    addHandle = driver.find_element(By.CLASS_NAME, "widget-as-image-bg").find_element(By.CLASS_NAME, "clear-button")
    addHandle.click()
except NoSuchElementException:
    print("No adds")

try:
    cookieHandle = driver.find_element(By.ID, "gdpr-wrapper-new").find_element(By.CLASS_NAME, "accept-button")
    cookieHandle.click()
except NoSuchElementException:
    print("No cookies")

soup = BeautifulSoup(driver.execute_script("return document.body.innerHTML;"), "html.parser")

footballDiv = soup.find("div", class_="sportsoffer dark")
competitions = footballDiv.find_all("div", class_="competition")

file = open("Findings.txt", "w", encoding="utf-8")
file.close()

competitionCount = 0
articleCount = 0
versusCount = 0

footballDivElement = driver.find_element(By.CSS_SELECTOR, ".sportsoffer")
competitionDivElements = footballDivElement.find_elements(By.CLASS_NAME, "competition")

for competition in competitions:
    articles = competition.find_all("article")
    articleElements = competitionDivElements[competitionCount].find_elements(By.TAG_NAME, "article")
    if articles != NULL:
        articleCount = 0
        for article in articles:
            teamVersus = article.find("div", class_="part1").find("a", class_="pairs").find_all("span") 
            if len(teamVersus) != 3:
                articleCount += 1
                continue

            statsButtonElement = articleElements[articleCount].find_element(By.CLASS_NAME, "stats")            
            statsButtonElement.click()
            
            for handle in driver.window_handles:
                if handle != originalWindow:
                   driver.switch_to.window(handle)
                   driver.minimize_window()
                   break
              
            mecevaPob = [0, 0]
            ukupnoGolova = [0, 0]
            golovaProsek= [0, 0]
            
            tempDriver = requests.get(driver.current_url)
            soup = BeautifulSoup(tempDriver.text, "html.parser")
            tempDriver.close()
            if soup.title.text != "Statistics":
                tables = soup.find_all("div", class_="col-sm-5 graphics-text-regular-color")
                if tables != NoneType:
                    mecevaPob[0] = tables[0].find("div", class_="h1 no-margin").find("strong").text
                    ukupnoGolova[0] = tables[0].find_all("div", class_="margin-top")[2].find("strong").text
                    golovaProsek[0] = tables[0].find_all("div", class_="margin-top")[3].find("strong").text
                    mecevaPob[1] = tables[1].find("div", class_="h1 no-margin").find("strong").text
                    ukupnoGolova[1] = tables[1].find_all("div", class_="margin-top")[2].find("strong").text
                    golovaProsek[1] = tables[1].find_all("div", class_="margin-top")[3].find("strong").text

            file = open("Findings.txt", "a", encoding="utf-8")
            file.write(str(versusCount) + "|" + teamVersus[1].text + "|" + str(mecevaPob[0]) + "|" + str(ukupnoGolova[0]) + "|" + str(golovaProsek[0]) + "\n")
            file.write(str(versusCount) + "|" + teamVersus[2].text + "|" + str(mecevaPob[1]) + "|" + str(ukupnoGolova[1]) + "|" + str(golovaProsek[1]) + "\n")
            file.close()
            driver.close()
            driver.switch_to.window(originalWindow)
            driver.minimize_window()
            versusCount += 1
            articleCount += 1
    competitionCount += 1